#include "hwinfo/monitor.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
const std::string& Monitor::vendor() const { return _vendor; }

// _____________________________________________________________________________________________________________________
const std::string& Monitor::model() const { return _model; }

// _____________________________________________________________________________________________________________________
const std::string& Monitor::resolution() const { return _resolution; }

// _____________________________________________________________________________________________________________________
const std::string& Monitor::refreshRate() const { return _refreshRate; }

// _____________________________________________________________________________________________________________________
const std::string& Monitor::serialNumber() const { return _serialNumber; }

}  // namespace hwinfo
