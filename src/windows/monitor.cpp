#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS
#include <hwinfo/monitor.h>

// clang-format off
#include <Windows.h>       // Must be included before SetupAPI.h to avoid dependency issues.
#include <stringapiset.h>
#include <SetupAPI.h>       // Requires Windows.h to be included first.
#include <devguid.h>
// clang-format on

#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "Setupapi.lib")

namespace {

// Convert char* (ANSI) to std::wstring (Wide)
std::wstring ConvertToWideString(const char* input) {
  int size_needed = MultiByteToWideChar(CP_ACP, 0, input, -1, NULL, 0);
  std::wstring result(size_needed, 0);
  MultiByteToWideChar(CP_ACP, 0, input, -1, &result[0], size_needed);
  return result;
}

// Convert wchar_t* to std::string (Narrow)
std::string ConvertToNarrowString(const wchar_t* input) {
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
  std::string result(size_needed - 1, 0);  // Remove extra null character
  WideCharToMultiByte(CP_UTF8, 0, input, -1, &result[0], size_needed, NULL, NULL);
  return result;
}

// Convert char* to std::string (Wide to Narrow conversion)
std::string ConvertCharArrayToString(const char* input) {
  std::wstring wideStr = ConvertToWideString(input);
  return ConvertToNarrowString(wideStr.c_str());
}

// Function to get monitor serial number
std::string GetMonitorSerialNumber(int index) {
  HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_MONITOR, NULL, NULL, DIGCF_PRESENT);
  if (hDevInfo == INVALID_HANDLE_VALUE) {
    return "<unknown>";
  }

  SP_DEVINFO_DATA DeviceInfoData;
  DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
  char serialNumber[256] = {0};
  DWORD size;

  if (SetupDiEnumDeviceInfo(hDevInfo, index, &DeviceInfoData) &&
      SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)serialNumber,
                                       sizeof(serialNumber), &size)) {
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return std::string(serialNumber);
  }

  SetupDiDestroyDeviceInfoList(hDevInfo);
  return "<unknown>";
}

}  // namespace

namespace hwinfo {

// Get all connected monitors
std::vector<Monitor> getAllMonitors() {
  std::vector<Monitor> monitors;

  int deviceIndex = 0;
  DISPLAY_DEVICE displayDevice;
  displayDevice.cb = sizeof(DISPLAY_DEVICE);

  while (EnumDisplayDevices(NULL, deviceIndex, &displayDevice, 0)) {
    if (displayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE) {
      std::string vendor = ConvertCharArrayToString(displayDevice.DeviceString);

      DISPLAY_DEVICE monitorDevice;
      monitorDevice.cb = sizeof(DISPLAY_DEVICE);

      std::string model = "<unknown>";
      if (EnumDisplayDevices(displayDevice.DeviceName, 0, &monitorDevice, 0)) {
        model = ConvertCharArrayToString(monitorDevice.DeviceString);
      }

      DEVMODE devMode;
      devMode.dmSize = sizeof(DEVMODE);
      std::string resolution = "<unknown>";
      std::string refreshRateStr = "<unknown>";

      if (EnumDisplaySettings(displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &devMode)) {
        resolution = std::to_string(devMode.dmPelsWidth) + "x" + std::to_string(devMode.dmPelsHeight);
        refreshRateStr = std::to_string(devMode.dmDisplayFrequency);
      }

      std::string serialNumber = GetMonitorSerialNumber(deviceIndex);
      monitors.emplace_back(vendor, model, resolution, refreshRateStr, serialNumber);
    }
    deviceIndex++;
  }

  return monitors;
}

}  // namespace hwinfo

#endif