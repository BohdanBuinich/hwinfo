#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX
#include <hwinfo/monitor.h>
#include <hwinfo/utils/filesystem.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {
constexpr size_t EDID_LENGTH = 128;
const std::string DRM_PATH = "/sys/class/drm/";

// Convert 2-byte manufacturer ID to a readable string
std::string decodeManufacturer(uint16_t raw) {
  return {static_cast<char>(((raw >> 10) & 0x1F) + 'A' - 1), static_cast<char>(((raw >> 5) & 0x1F) + 'A' - 1),
          static_cast<char>((raw & 0x1F) + 'A' - 1)};
}

// Read EDID binary data from the given path
std::optional<std::vector<uint8_t>> readEDID(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return std::nullopt;

  std::vector<uint8_t> edid(std::istreambuf_iterator<char>(file), {});
  return (edid.size() >= EDID_LENGTH) ? std::optional<std::vector<uint8_t>>(std::move(edid)) : std::nullopt;
}

// Parse EDID data into a `Monitor` object
hwinfo::Monitor parseEDID(const std::vector<uint8_t>& edid) {
  if (edid.size() < EDID_LENGTH) {
    return {"<unknown>", "<unknown>", "<unknown>", "<unknown>", "<unknown>"};
  }

  std::string manufacturer = decodeManufacturer((edid[8] << 8) | edid[9]);
  std::string model = std::to_string((edid[11] << 8) | edid[10]);

  uint32_t serial = (edid[12] | (edid[13] << 8) | (edid[14] << 16) | (edid[15] << 24));
  std::string serialStr = (serial == 0) ? "<unknown>" : std::to_string(serial);

  std::string size = std::to_string(edid[21] * 10) + "x" + std::to_string(edid[22] * 10) + " mm";

  uint16_t h_active = (edid[56] | ((edid[58] & 0xF0) << 4));
  uint16_t v_active = (edid[59] | ((edid[61] & 0xF0) << 4));
  std::string resolution = std::to_string(h_active) + "x" + std::to_string(v_active);

  uint16_t h_blank = (edid[57] | ((edid[58] & 0x0F) << 8));
  uint16_t v_blank = (edid[60] | ((edid[61] & 0x0F) << 8));
  uint16_t pixel_clock = (edid[54] | (edid[55] << 8));

  std::string refreshRate = "<unknown>";
  if (pixel_clock > 0) {
    auto refresh_rate =
        static_cast<uint16_t>(std::round((pixel_clock * 10000.0) / ((h_active + h_blank) * (v_active + v_blank))));
    refreshRate = std::to_string(refresh_rate);
  }

  return {manufacturer, model, resolution, refreshRate, serialStr};
}

}  // namespace

namespace hwinfo {

// Get all connected monitors
std::vector<Monitor> getAllMonitors() {
  std::vector<Monitor> monitors;

  for (const auto& name : filesystem::getDirectoryEntries(DRM_PATH)) {
    if (name.rfind("card", 0) == 0 &&
        (name.find("eDP-") != std::string::npos || name.find("HDMI-") != std::string::npos ||
         name.find("DP-") != std::string::npos)) {
      std::string edidPath = DRM_PATH + name + "/edid";

      if (auto edidData = readEDID(edidPath); edidData && !edidData->empty()) {
        monitors.push_back(parseEDID(*edidData));
      }
    }
  }

  return monitors;
}

}  // namespace hwinfo

#endif
