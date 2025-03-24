#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <string>
#include <unordered_map>
#include <vector>

#include "hwinfo/network.h"
#include "hwinfo/utils/constants.h"
#include "hwinfo/utils/stringutils.h"
#include "hwinfo/utils/wmi_wrapper.h"

namespace hwinfo {

namespace {

inline std::string adapterTypeIDToString(unsigned short typeID, const std::wstring& name,
                                         const std::wstring& pnpDeviceID /* optional */) {
  switch (typeID) {
    case 0:
      if (name.find(L"Microsoft Hyper-V Network Adapter") != std::wstring::npos) return "Hyper-V Virtual Adapter";
      if (name.find(L"Hyper-V") != std::wstring::npos) return "Hyper-V Virtual Adapter";
      if (name.find(L"Kernel Debug") != std::wstring::npos) return "Kernel Debug Adapter";
      if (name.find(L"Switch") != std::wstring::npos) return "Virtual Switch Adapter";
      return "Ethernet";
    case 9:
      return "WiFi";
    default:
      break;
  }

  if (name.find(L"Loopback") != std::wstring::npos) return "Loopback";
  if (name.find(L"TAP-Windows") != std::wstring::npos || name.find(L"TUN") != std::wstring::npos) return "TUN/TAP";
  if (name.find(L"Bridge") != std::wstring::npos) return "Bridge";
  if (name.find(L"Hyper-V") != std::wstring::npos) return "Hyper-V Virtual Adapter";
  if (pnpDeviceID.find(L"USB") != std::wstring::npos || name.find(L"USB") != std::wstring::npos) return "USB Ethernet";

  return constants::UNKNOWN;
}

/**
 * @brief Query Win32_NetworkAdapter for each adapter's Index and AdapterTypeID,
 *        build a map from adapter index -> string type (Ethernet, Wireless, etc.).
 */
std::unordered_map<int, std::string> getWindowsAdapterTypes() {
  std::unordered_map<int, std::string> adapterTypes;
  utils::WMI::_WMI wmi;

  const std::wstring queryString =
      L"SELECT Index, AdapterTypeID, Name, PNPDeviceID, InterfaceIndex "
      L"FROM Win32_NetworkAdapter";

  if (!wmi.execute_query(queryString)) return adapterTypes;

  IWbemClassObject* wbemObject = nullptr;
  ULONG returnedCount = 0;

  while (wmi.enumerator) {
    HRESULT hr = wmi.enumerator->Next(WBEM_INFINITE, 1, &wbemObject, &returnedCount);
    if (FAILED(hr) || !returnedCount) break;

    VARIANT vtIndex, vtTypeID, vtName, vtPNPDeviceID, vtInterfaceIndex;
    VariantInit(&vtIndex);
    VariantInit(&vtTypeID);
    VariantInit(&vtName);
    VariantInit(&vtPNPDeviceID);
    VariantInit(&vtInterfaceIndex);

    HRESULT hrIndex = wbemObject->Get(L"Index", 0, &vtIndex, nullptr, nullptr);
    HRESULT hrTypeID = wbemObject->Get(L"AdapterTypeID", 0, &vtTypeID, nullptr, nullptr);
    HRESULT hrName = wbemObject->Get(L"Name", 0, &vtName, nullptr, nullptr);
    HRESULT hrPNP = wbemObject->Get(L"PNPDeviceID", 0, &vtPNPDeviceID, nullptr, nullptr);
    HRESULT hrInterfaceIndex = wbemObject->Get(L"InterfaceIndex", 0, &vtInterfaceIndex, nullptr, nullptr);

    if (SUCCEEDED(hrIndex)) {
      int index = static_cast<int>(vtIndex.ulVal);
      unsigned short typeID = SUCCEEDED(hrTypeID) ? vtTypeID.uiVal : 0;

      std::wstring adapterName = SUCCEEDED(hrName) ? std::wstring(vtName.bstrVal, SysStringLen(vtName.bstrVal)) : L"";
      std::wstring wpnpid =
          SUCCEEDED(hrPNP) ? std::wstring(vtPNPDeviceID.bstrVal, SysStringLen(vtPNPDeviceID.bstrVal)) : L"";

      std::string typeStr = adapterTypeIDToString(typeID, adapterName, wpnpid);

      if (SUCCEEDED(hrInterfaceIndex) && vtInterfaceIndex.uintVal > 0) {
        int interfaceIndex = static_cast<int>(vtInterfaceIndex.uintVal);
        adapterTypes[interfaceIndex] = typeStr;
      }
    }

    VariantClear(&vtIndex);
    VariantClear(&vtTypeID);
    VariantClear(&vtName);
    VariantClear(&vtPNPDeviceID);
    VariantClear(&vtInterfaceIndex);

    wbemObject->Release();
  }

  return adapterTypes;
}
}  // namespace

// _____________________________________________________________________________________________________________________
std::vector<Network> getAllNetworks() {
  auto adapterTypeMap = getWindowsAdapterTypes();

  utils::WMI::_WMI wmi;
  const std::wstring queryString =
      L"SELECT InterfaceIndex, IPAddress, Description, MACAddress "
      L"FROM Win32_NetworkAdapterConfiguration";

  if (!wmi.execute_query(queryString)) {
    return {};
  }

  std::vector<Network> networks;
  IWbemClassObject* wbemObject = nullptr;
  ULONG returnedCount = 0;

  while (wmi.enumerator) {
    HRESULT hr = wmi.enumerator->Next(WBEM_INFINITE, 1, &wbemObject, &returnedCount);
    if (FAILED(hr) || !returnedCount) {
      break;
    }

    Network network;
    VARIANT vtProp;
    VariantInit(&vtProp);

    // --------------------------------------------------
    //  InterfaceIndex
    // --------------------------------------------------
    hr = wbemObject->Get(L"InterfaceIndex", 0, &vtProp, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      network._index = std::to_string(vtProp.uintVal);
    }
    VariantClear(&vtProp);

    // --------------------------------------------------
    //  IPAddress (array of BSTR with IPv4 & IPv6)
    // --------------------------------------------------
    VariantInit(&vtProp);
    hr = wbemObject->Get(L"IPAddress", 0, &vtProp, nullptr, nullptr);
    if (SUCCEEDED(hr) && (vtProp.vt == (VT_ARRAY | VT_BSTR))) {
      LONG lowerBound = 0, upperBound = 0;
      SafeArrayGetLBound(vtProp.parray, 1, &lowerBound);
      SafeArrayGetUBound(vtProp.parray, 1, &upperBound);

      std::string ipv4, ipv6;
      for (LONG i = lowerBound; i <= upperBound; ++i) {
        BSTR bstrIp = nullptr;
        if (SUCCEEDED(SafeArrayGetElement(vtProp.parray, &i, &bstrIp)) && bstrIp) {
          std::wstring wsIp(bstrIp, SysStringLen(bstrIp));
          std::string ip = utils::wstring_to_std_string(wsIp);

          // Distinguish IPv6 vs IPv4
          if (ip.find(':') != std::string::npos) {
            // If you only want link-local IPv6, check for "fe80::".
            if (ip.rfind("fe80::", 0) == 0) {
              ipv6 = ip;
            } else {
              // If you want to capture a global IPv6 instead, handle it here.
              // e.g., ipv6 = ip;
            }
          } else {
            ipv4 = ip;
          }
          SysFreeString(bstrIp);
        }
      }
      network._ip4 = ipv4;
      network._ip6 = ipv6;
    }
    VariantClear(&vtProp);

    // --------------------------------------------------
    //  Description
    // --------------------------------------------------
    VariantInit(&vtProp);
    hr = wbemObject->Get(L"Description", 0, &vtProp, nullptr, nullptr);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
      network._description = utils::wstring_to_std_string(vtProp.bstrVal);
    }
    VariantClear(&vtProp);

    // --------------------------------------------------
    //  MACAddress
    // --------------------------------------------------
    VariantInit(&vtProp);
    hr = wbemObject->Get(L"MACAddress", 0, &vtProp, nullptr, nullptr);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
      network._mac = utils::wstring_to_std_string(vtProp.bstrVal);
    }
    VariantClear(&vtProp);

    // --------------------------------------------------
    //  Fill in the _type field using our adapterTypeMap
    // --------------------------------------------------
    if (!network._index.empty()) {
      try {
        int idx = std::stoi(network._index);
        if (auto it = adapterTypeMap.find(idx); it != adapterTypeMap.end()) {
          network._type = it->second;
        } else {
          network._type = constants::UNKNOWN;
        }
      } catch (...) {
        // Could not parse index; leave type as unknown
        network._type = constants::UNKNOWN;
      }
    } else {
      network._type = constants::UNKNOWN;
    }

    // Release WMI object for this iteration
    wbemObject->Release();
    wbemObject = nullptr;

    networks.push_back(std::move(network));
  }

  return networks;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS