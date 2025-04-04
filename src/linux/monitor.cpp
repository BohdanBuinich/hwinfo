#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX
#include <dirent.h>
#include <fcntl.h>
#include <hwinfo/monitor.h>
#include <unistd.h>
#include <xf86drmMode.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <vector>

constexpr auto EDID_LENGTH = 128;

namespace drm_util {
class DRMDevice {
 public:
  explicit DRMDevice(const std::string& path) : fd_(open(path.c_str(), O_RDWR)) {}

  ~DRMDevice() {
    if (fd_ >= 0) close(fd_);
  }

  [[nodiscard]] int getFD() const { return fd_; }
  [[nodiscard]] bool isValid() const { return fd_ >= 0; }

 private:
  int fd_;
};

// Get all available DRM card devices dynamically
std::vector<std::string> getAvailableDRICards() {
  const std::string dri_path = "/dev/dri/";
  std::vector<std::string> dri_cards;

  if (DIR* dir = opendir(dri_path.c_str())) {
    const std::regex card_regex("^card[0-9]+$");
    while (dirent* entry = readdir(dir)) {
      if (std::regex_match(entry->d_name, card_regex)) {
        dri_cards.emplace_back(dri_path + entry->d_name);
      }
    }
    closedir(dir);
  }
  return dri_cards;
}

// Read EDID data from a file
std::optional<std::vector<uint8_t>> readEDID(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return std::nullopt;

  return std::vector<uint8_t>{std::istreambuf_iterator<char>(file), {}};
}

// Extract Vendor from EDID
std::string getVendorFromEDID(const std::vector<uint8_t>& edid) {
  if (edid.size() < EDID_LENGTH) return "<unknown>";

  return {static_cast<char>(((edid[8] >> 2) & 0x1F) + 'A' - 1),
          static_cast<char>(((edid[8] & 0x03) << 3 | (edid[9] >> 5)) + 'A' - 1),
          static_cast<char>((edid[9] & 0x1F) + 'A' - 1)};
}

// Extract Model & Serial from EDID
std::pair<std::string, std::string> getModelAndSerialFromEDID(const std::vector<uint8_t>& edid) {
  if (edid.size() < EDID_LENGTH) return {"<unknown>", "<unknown>"};

  std::string model = std::to_string((edid[11] << 8) | edid[10]);
  std::string serialNumber = std::to_string(edid[12] | (edid[13] << 8) | (edid[14] << 16) | (edid[15] << 24));

  return {model, serialNumber};
}

std::pair<std::string, std::string> getResolutionAndRefreshRateFromEDID(const std::vector<uint8_t>& edid) {
  if (edid.size() < EDID_LENGTH) {
    return {"<unknown>", "<unknown>"};
  }

  // Read Active Pixels
  const auto h_active = static_cast<uint16_t>(edid[56] | ((edid[58] & 0xF0) << 4));
  const auto v_active = static_cast<uint16_t>(edid[59] | ((edid[61] & 0xF0) << 4));

  if (h_active == 0 || v_active == 0) {
    return {"<unknown>", "<unknown>"};
  }

  // Generate Resolution String
  std::string resolution = std::to_string(h_active) + "x" + std::to_string(v_active);

  // Read Blank Pixels
  const auto h_blank = static_cast<uint16_t>(edid[57] | ((edid[58] & 0x0F) << 8));
  const auto v_blank = static_cast<uint16_t>(edid[60] | ((edid[61] & 0x0F) << 8));

  // Read Pixel Clock (in 10 kHz units)
  const auto pixel_clock = static_cast<uint16_t>(edid[54] | (edid[55] << 8));

  // Check if pixel clock is valid
  if (pixel_clock == 0) {
    return {resolution, "<unknown>"};
  }

  // Calculate Refresh Rate (in Hz)
  const double refresh_rate = (pixel_clock * 10000.0) / ((h_active + h_blank) * (v_active + v_blank));
  std::string refreshRate = std::to_string(static_cast<int>(std::round(refresh_rate)));

  return {resolution, refreshRate};
}

// Get Connector Type Name from DRM mode
std::string getConnectorTypeName(uint32_t type) {
  switch (type) {
    case DRM_MODE_CONNECTOR_VGA:
      return "VGA";
    case DRM_MODE_CONNECTOR_DVII:
      return "DVI-I";
    case DRM_MODE_CONNECTOR_DVID:
      return "DVI-D";
    case DRM_MODE_CONNECTOR_DVIA:
      return "DVI-A";
    case DRM_MODE_CONNECTOR_Composite:
      return "Composite";
    case DRM_MODE_CONNECTOR_SVIDEO:
      return "S-Video";
    case DRM_MODE_CONNECTOR_LVDS:
      return "LVDS";
    case DRM_MODE_CONNECTOR_Component:
      return "Component";
    case DRM_MODE_CONNECTOR_9PinDIN:
      return "9PinDIN";
    case DRM_MODE_CONNECTOR_DisplayPort:
      return "DP";
    case DRM_MODE_CONNECTOR_HDMIA:
      return "HDMI-A";
    case DRM_MODE_CONNECTOR_HDMIB:
      return "HDMI-B";
    case DRM_MODE_CONNECTOR_TV:
      return "TV";
    case DRM_MODE_CONNECTOR_eDP:
      return "eDP";
    case DRM_MODE_CONNECTOR_VIRTUAL:
      return "Virtual";
    case DRM_MODE_CONNECTOR_DSI:
      return "DSI";
    default:
      return "<unknown>";
  }
}

}  // namespace drm_util

namespace hwinfo {

// Get all connected monitors
std::vector<Monitor> getAllMonitors() {
  std::vector<Monitor> monitors;

  for (const auto& dri_path : drm_util::getAvailableDRICards()) {
    drm_util::DRMDevice drmDevice(dri_path);
    if (!drmDevice.isValid()) continue;

    // Use smart pointer for drmModeRes with custom deleter
    const std::unique_ptr<drmModeRes, decltype(&drmModeFreeResources)> res(drmModeGetResources(drmDevice.getFD()),
                                                                           drmModeFreeResources);

    if (!res) continue;

    for (int i = 0; i < res->count_connectors; ++i) {
      // Use smart pointer for drmModeConnector with custom deleter
      const std::unique_ptr<drmModeConnector, decltype(&drmModeFreeConnector)> conn(
          drmModeGetConnector(drmDevice.getFD(), res->connectors[i]), drmModeFreeConnector);

      if (!conn) continue;

      if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
        std::string connector_name = dri_path.substr(dri_path.find_last_of('/') + 1) + "-" +
                                     drm_util::getConnectorTypeName(conn->connector_type) + "-" +
                                     std::to_string(conn->connector_type_id);

        std::string edid_path = "/sys/class/drm/" + connector_name + "/edid";
        auto edidData = drm_util::readEDID(edid_path);

        std::string vendor = edidData ? drm_util::getVendorFromEDID(*edidData) : "<unknown>";
        auto [model, serialNumber] =
            edidData ? drm_util::getModelAndSerialFromEDID(*edidData) : std::make_pair("<unknown>", "<unknown>");

        auto [drmResolution, drmRefreshRate] = drm_util::getResolutionAndRefreshRateFromEDID(*edidData);

        // Determine resolution
        std::string resolution = (drmResolution == "<unknown>") ? std::to_string(conn->modes[0].hdisplay) + "x" +
                                                                      std::to_string(conn->modes[0].vdisplay)
                                                                : drmResolution;

        // Determine refresh rate
        std::string refreshRate =
            (drmRefreshRate == "<unknown>") ? std::to_string(conn->modes[0].vrefresh) : drmRefreshRate;

        monitors.emplace_back(vendor, model, resolution, refreshRate, serialNumber);
      }
    }
  }
  return monitors;
}

}  // namespace hwinfo

#endif
