#pragma once

#include <hwinfo/platform.h>

#include <string>
#include <vector>

namespace hwinfo {

class HWINFO_API Monitor {
 public:
  Monitor(std::string vendor, std::string name, std::string resolution, std::string refreshRate,
          std::string serialNumber)
      : _vendor(std::move(vendor)),
        _model(std::move(name)),
        _resolution(std::move(resolution)),
        _refreshRate(std::move(refreshRate)),
        _serialNumber(std::move(serialNumber)) {}

  ~Monitor() = default;

  HWI_NODISCARD const std::string& vendor() const;
  HWI_NODISCARD const std::string& model() const;
  HWI_NODISCARD const std::string& resolution() const;
  HWI_NODISCARD const std::string& refreshRate() const;
  HWI_NODISCARD const std::string& serialNumber() const;

 private:
  std::string _vendor{"Unknown"};
  std::string _model{"Unknown"};
  std::string _resolution{"Unknown"};
  std::string _refreshRate{"Unknown"};
  std::string _serialNumber{"Unknown"};
};

std::vector<Monitor> getAllMonitors();

}  // namespace hwinfo
