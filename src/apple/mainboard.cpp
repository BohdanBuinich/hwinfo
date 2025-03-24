// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include "hwinfo/mainboard.h"
#include "hwinfo/utils/constants.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
MainBoard::MainBoard() {
  _vendor = constants::UNKNOWN;
  _name = constants::UNKNOWN;
  _version = constants::UNKNOWN;
  _serialNumber = constants::UNKNOWN;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE