#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <hwinfo/monitor.h>

#include <iostream>
#include <string>
#include <vector>

namespace hwinfo {

// Get all connected monitors
std::vector<Monitor> getAllMonitors() {
  std::vector<Monitor> monitors;

  uint32_t displayCount;
  CGGetOnlineDisplayList(0, NULL, &displayCount);  // Get display count

  if (displayCount == 0) {
    std::cout << "No displays found!" << std::endl;  // TODO: remvoe
    return monitors;
  }

  CGDirectDisplayID displays[displayCount];
  CGGetOnlineDisplayList(displayCount, displays, &displayCount);  // Get display IDs

  for (auto displayID : displays) {
    // Get Vendor ID
    // uint32_t vendorID = CGDisplayVendorNumber(displayID);
    // std::string vendor = (vendorID == 0) ? "<unknown>" : std::to_string(vendorID);

    // Get Model ID
    uint32_t modelID = CGDisplayModelNumber(displayID);
    std::string model = (modelID == 0) ? "<unknown>" : std::to_string(modelID);

    // Get Serial Number
    uint32_t serial = CGDisplaySerialNumber(displayID);
    std::string serialNumber = (serial == 0) ? "<unknown>" : std::to_string(serial);

    // Get Resolution
    CGDisplayModeRef mode = CGDisplayCopyDisplayMode(displayID);
    uint32_t width = CGDisplayModeGetWidth(mode);
    uint32_t height = CGDisplayModeGetHeight(mode);
    std::string resolution = std::to_string(width) + "x" + std::to_string(height);

    // Get Refresh Rate
    uint32_t refreshRateValue = std::round(CGDisplayModeGetRefreshRate(mode));
    std::string refreshRate = (refreshRateValue > 0) ? std::to_string(refreshRateValue) : "<unknown>";

    // Clean up
    if (mode) {
      CGDisplayModeRelease(mode);
    }

    monitors.emplace_back("<unknown>", model, resolution, refreshRate, serialNumber);
  }

  return monitors;
}

}  // namespace hwinfo

#endif
