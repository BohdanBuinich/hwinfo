#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "hwinfo/network.h"
#include "hwinfo/utils/constants.h"

namespace hwinfo {

namespace {

/**
 * @brief Get a user-friendly description of the interface.
 *        Here it simply returns the interface name itself.
 */
std::string getDescription(const std::string& path) { return path; }

/**
 * @brief Returns the interface index as a string, or "<unknown>" if invalid.
 */
std::string getInterfaceIndex(const std::string& iface) {
  unsigned int index = if_nametoindex(iface.c_str());
  return (index > 0) ? std::to_string(index) : constants::UNKNOWN;
}

/**
 * @brief Reads MAC address from sysfs or returns "<unknown>" if unavailable.
 */
std::string getMac(const std::string& iface) {
  const std::string path = "/sys/class/net/" + iface + "/address";
  std::ifstream ifs(path);
  if (!ifs) {
    return constants::UNKNOWN;
  }

  std::string mac;
  std::getline(ifs, mac);
  return mac.empty() ? constants::UNKNOWN : mac;
}

/**
 * @brief Generic IP address fetcher (IPv4 or IPv6).
 * @param iface   Interface name (e.g., "eth0").
 * @param family  Address family (AF_INET for IPv4, AF_INET6 for IPv6).
 * @return The first matching IP address as a string, or "<unknown>" if not found.
 */
std::string getIp(const std::string& iface, int family) {
  // Use RAII to ensure freeifaddrs is always called.
  struct ifaddrs* rawAddrs = nullptr;
  if (getifaddrs(&rawAddrs) == -1) {
    return constants::UNKNOWN;
  }
  std::unique_ptr<struct ifaddrs, decltype(&freeifaddrs)> ifAddrs(rawAddrs, freeifaddrs);

  for (auto* ifa = rawAddrs; ifa != nullptr; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr || iface != ifa->ifa_name) {
      continue;
    }

    if (ifa->ifa_addr->sa_family == family) {
      char buffer[INET6_ADDRSTRLEN] = {0};

      if (family == AF_INET) {
        auto* addr = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
        if (inet_ntop(AF_INET, &addr->sin_addr, buffer, sizeof(buffer))) {
          return buffer;
        }
      } else if (family == AF_INET6) {
        auto* addr = reinterpret_cast<struct sockaddr_in6*>(ifa->ifa_addr);
        if (inet_ntop(AF_INET6, &addr->sin6_addr, buffer, sizeof(buffer))) {
          // Here we exemplify preferring link-local if found (fe80::).
          // Adjust logic if you need the first global address, etc.
          if (std::strncmp(buffer, "fe80", 4) == 0) {
            return buffer;
          }
          // If you *only* want link-local, return immediately.
          // Otherwise, you could store the address and keep searching
          // in case you prefer global addresses. For now, we return
          // upon finding link-local.
        }
      }
    }
  }

  return constants::UNKNOWN;
}

/**
 * @brief Helper to retrieve the IPv4 address of an interface.
 */
std::string getIp4(const std::string& iface) { return getIp(iface, AF_INET); }

/**
 * @brief Helper to retrieve the IPv6 address of an interface.
 */
std::string getIp6(const std::string& iface) { return getIp(iface, AF_INET6); }

/**
 * @brief Attempts to determine the interface type:
 *        - WiFi
 *        - Loopback
 *        - USB Ethernet
 *        - Bridge
 *        - TUN/TAP
 *        - Ethernet
 *        - <unknown> otherwise
 */
std::string getInterfaceType(const std::string& iface) {
  const std::string sysPath = "/sys/class/net/" + iface;

  // Check for Wi-Fi interface
  if (std::filesystem::exists(sysPath + "/wireless")) {
    return "WiFi";
  }

  // Check for Loopback
  {
    std::ifstream typeFile(sysPath + "/type");
    int typeVal = 0;
    if (typeFile.is_open()) {
      typeFile >> typeVal;
    }
    if (typeVal == 772 || iface == "lo") {
      return "Loopback";
    }
    // typeVal == 1 typically means Ethernet, but we check further below.
  }

  // Check for USB-based Ethernet via module path
  {
    std::error_code ec;
    auto modulePath = std::filesystem::read_symlink(sysPath + "/device/driver/module", ec);
    if (!ec && modulePath.string().find("usb") != std::string::npos) {
      return "USB Ethernet";
    }
  }

  // Check for USB-based via device symlink
  {
    std::error_code ec;
    auto deviceLink = std::filesystem::read_symlink(sysPath + "/device", ec);
    if (!ec && deviceLink.string().find("usb") != std::string::npos) {
      return "USB Ethernet";
    }
  }

  // Check for Bridge Interface
  if (std::filesystem::exists(sysPath + "/bridge")) {
    return "Bridge";
  }

  // Check for TUN/TAP
  if (std::filesystem::exists(sysPath + "/tun_flags")) {
    return "TUN/TAP";
  }

  // If none of the above checks matched, try reading "type" again:
  {
    std::ifstream typeFile(sysPath + "/type");
    int typeVal = 0;
    if (typeFile.is_open()) {
      typeFile >> typeVal;
    }
    // 1 often represents a standard Ethernet interface.
    if (typeVal == 1) {
      return "Ethernet";
    }
  }

  return constants::UNKNOWN;
}

}  // namespace

/**
 * @brief Collect all network interfaces with their info (index, MAC, IP, etc.).
 */
std::vector<Network> getAllNetworks() {
  std::vector<Network> networks;

  // Use RAII to ensure freeifaddrs is always called.
  ifaddrs* rawAddrs = nullptr;
  if (getifaddrs(&rawAddrs) == -1) {
    perror("getifaddrs");
    return networks;
  }
  std::unique_ptr<struct ifaddrs, decltype(&freeifaddrs)> ifAddrs(rawAddrs, freeifaddrs);

  // Loop over all interfaces
  for (auto* ifa = rawAddrs; ifa != nullptr; ifa = ifa->ifa_next) {
    // We only care about AF_PACKET for physical interfaces (MAC-based).
    if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_PACKET) {
      continue;
    }

    const std::string interfaceName = ifa->ifa_name;
    Network network;
    network._index = getInterfaceIndex(interfaceName);
    network._description = getDescription(interfaceName);
    network._mac = getMac(interfaceName);
    network._ip4 = getIp4(interfaceName);
    network._ip6 = getIp6(interfaceName);
    network._type = getInterfaceType(interfaceName);

    networks.push_back(std::move(network));
  }

  return networks;
}

}  // namespace hwinfo

#endif
