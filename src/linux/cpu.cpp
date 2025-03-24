// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <unistd.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "hwinfo/cpu.h"
#include "hwinfo/utils/filesystem.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
// Helper Function to Retrieve Frequency
int64_t getFrequencyFromPaths(const std::vector<std::string>& paths) {
  for (const auto& path : paths) {
    if (const int64_t Hz = filesystem::get_specs_by_file_path(path); Hz > -1) {
      return Hz / 1000;  // Convert from kHz to MHz
    }
  }
  return -1;  // Failed to retrieve frequency from all paths
}

// _____________________________________________________________________________________________________________________
// Get Max Clock Speed (MHz)
int64_t getMaxClockSpeed_MHz(const int& core_id) {
  const std::string basePath = "/sys/devices/system/cpu/cpu" + std::to_string(core_id) + "/cpufreq/";
  const std::string policyPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(core_id) + "/";

  const std::vector maxFrequencyPaths = {
      basePath + "scaling_max_freq",    // Intel & ARM (individual cores)
      basePath + "cpuinfo_max_freq",    // Intel (individual cores)
      policyPath + "scaling_max_freq",  // ARM (policy-based systems)
      policyPath + "cpuinfo_max_freq"   // ARM (policy-based systems)
  };

  return getFrequencyFromPaths(maxFrequencyPaths);
}

// _____________________________________________________________________________________________________________________
// Get Regular (Base) Clock Speed (MHz)
int64_t getRegularClockSpeed_MHz(const int& core_id) {
  const std::string basePath = "/sys/devices/system/cpu/cpu" + std::to_string(core_id) + "/cpufreq/";
  const std::string policyPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(core_id) + "/";

  const std::vector regularFrequencyPaths = {
      basePath + "base_frequency",      // Intel (individual cores)
      basePath + "scaling_cur_freq",    // ARM & Intel (individual cores, with scaling)
      basePath + "cpuinfo_cur_freq",    // ARM (individual cores, without scaling)
      policyPath + "scaling_cur_freq",  // ARM (policy-based systems)
      policyPath + "cpuinfo_cur_freq"   // ARM (policy-based systems)
  };

  return getFrequencyFromPaths(regularFrequencyPaths);
}

// _____________________________________________________________________________________________________________________
// Get Min Clock Speed (MHz)
int64_t getMinClockSpeed_MHz(const int& core_id) {
  const std::string basePath = "/sys/devices/system/cpu/cpu" + std::to_string(core_id) + "/cpufreq/";
  const std::string policyPath = "/sys/devices/system/cpu/cpufreq/policy" + std::to_string(core_id) + "/";

  const std::vector minFrequencyPaths = {
      basePath + "scaling_min_freq",    // Intel & ARM (individual cores)
      basePath + "cpuinfo_min_freq",    // Intel (individual cores)
      policyPath + "scaling_min_freq",  // ARM (policy-based systems)
      policyPath + "cpuinfo_min_freq"   // ARM (policy-based systems)
  };

  return getFrequencyFromPaths(minFrequencyPaths);
}

// _____________________________________________________________________________________________________________________
// Get Current Clock Speeds (MHz) for All Cores
std::vector<int64_t> CPU::currentClockSpeed_MHz() const {
  std::vector<int64_t> res;

  if (numLogicalCores() <= 0) {
    return res;
  }

  res.reserve(numLogicalCores());

  for (int core_id = 0; core_id < numLogicalCores(); ++core_id) {
    std::string freqPath = "/sys/devices/system/cpu/cpu" + std::to_string(core_id) + "/cpufreq/scaling_cur_freq";
    int64_t frequency_Hz = filesystem::get_specs_by_file_path(freqPath);

    if (frequency_Hz == -1) {
      continue;  // Continue checking other cores, don't break
    }

    res.push_back(frequency_Hz / 1000);
  }

  return res;
}

// _____________________________________________________________________________________________________________________
double CPU::currentUtilisation() const {
  init_jiffies();
  // TODO: Leon Freist a socket max num and a socket id inside the CPU could make it work with all sockets
  //       I will not support it because I only have a 1 socket target device
  static Jiffies last = Jiffies();

  Jiffies current = filesystem::get_jiffies(0);

  auto total_over_period = static_cast<double>(current.all - last.all);
  auto work_over_period = static_cast<double>(current.working - last.working);

  last = current;

  const double utilization = work_over_period / total_over_period;
  if (utilization < 0 || utilization > 1 || std::isnan(utilization)) {
    return -1.0;
  }
  return utilization;
}

// _____________________________________________________________________________________________________________________
double CPU::threadUtilisation(int thread_index) const {
  init_jiffies();
  // TODO: Leon Freist a socket max num and a socket id inside the CPU could make it work with all sockets
  //       I will not support it because I only have a 1 socket target device
  static std::vector<Jiffies> last(0);
  if (last.empty()) {
    last.resize(_numLogicalCores);
  }

  Jiffies current = filesystem::get_jiffies(thread_index + 1);  // thread_index works only with 1 socket right now

  auto total_over_period = static_cast<double>(current.all - last[thread_index].all);
  auto work_over_period = static_cast<double>(current.working - last[thread_index].working);

  last[thread_index] = current;

  const double percentage = work_over_period / total_over_period;
  if (percentage < 0 || percentage > 100 || std::isnan(percentage)) {
    return -1.0;
  }
  return percentage;
}

// _____________________________________________________________________________________________________________________
std::vector<double> CPU::threadsUtilisation() const {
  std::vector<double> thread_utility(CPU::_numLogicalCores);
  for (int thread_idx = 0; thread_idx < CPU::_numLogicalCores; ++thread_idx) {
    thread_utility[thread_idx] = threadUtilisation(thread_idx);
  }
  return thread_utility;
}

// _____________________________________________________________________________________________________________________
void CPU::init_jiffies() const {
  if (!_jiffies_initialized) {
    // Sleep 1 sec just for the start cause the usage needs to have a delta value which is depending on the unix file
    // read it's just for the init, you don't need to wait if the delta is already created ...
    std::this_thread::sleep_for(std::chrono::duration<double>(1));
    _jiffies_initialized = true;
  }
}

// CPU Temp -> Works | But requires Im_sensors
// double CPU::currentTemperature_Celsius() const {
//     if (!std::ifstream("/etc/sensors3.conf"))
//     {
//       std::cout << "The lm-sensors, the tool for monitoring your system's temperature, needs to be configured. Please
//       set it up." << '\n';
//       // Configure lm-sensors if not already configured
//       std::string detect_command = "sudo sensors-detect";
//       std::system(detect_command.c_str());
//     }

//     // TODO: Leon Freist a socket max num and a socket id inside the CPU could make it work with all sockets
//     //       I will not support it because I only have a 1 socket target device
//     const int Socked_id = 0;

//     // Command to get temperature data using 'sensors' command
//     std::string command = "sensors | grep 'Package id " + std::to_string(Socked_id) + "' | awk '{print $4}'";

//     // Open a pipe to execute the command and capture its output
//     FILE* pipe = popen(command.c_str(), "r");
//     if (!pipe) {
//         std::cerr << "Error executing command." << '\n';
//         return -1.0; // Return a negative value to indicate an error
//     }

//     char buffer[128];
//     std::string result = "";

//     // Read the output of the command into 'result'
//     while (!feof(pipe)) {
//         if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
//             result += buffer;
//         }
//     }

//     // Close the pipe
//     pclose(pipe);

//     // Convert the result (string) to a double
//     double temperature = -1.0; // Default value in case of conversion failure
//     std::istringstream(result) >> temperature;

//     return temperature;
// }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
// ARM Implementer Values Map
static const std::map<std::string, std::string> ARM_IMPLEMENTERS = {
    {"0x41", "ARM"},      {"0x42", "Broadcom"}, {"0x43", "Cavium"},
    {"0x44", "DEC"},      {"0x4e", "NVIDIA"},   {"0x50", "APM"},
    {"0x51", "Qualcomm"}, {"0x53", "Samsung"},  {"0x54", "Texas Instruments"},
    {"0x56", "Marvell"},  {"0x66", "Faraday"},  {"0x69", "Intel"}};

// ARM CPU Models
std::string getARMModelName(const std::string& implementer, const std::string& part_hex) {
  static const std::map<std::string, std::map<std::string, std::string>> arm_models = {
      {"0x41",
       {{"0x810", "ARM810"},
        {"0x920", "ARM920"},
        {"0x922", "ARM922"},
        {"0x926", "ARM926"},
        {"0x940", "ARM940"},
        {"0x946", "ARM946"},
        {"0x966", "ARM966"},
        {"0xa20", "ARM1020"},
        {"0xa22", "ARM1022"},
        {"0xa26", "ARM1026"},
        {"0xb02", "ARM11 MPCore"},
        {"0xb36", "ARM1136"},
        {"0xb56", "ARM1156"},
        {"0xb76", "ARM1176"},
        {"0xc05", "Cortex-A5"},
        {"0xc07", "Cortex-A7"},
        {"0xc08", "Cortex-A8"},
        {"0xc09", "Cortex-A9"},
        {"0xc0d", "Cortex-A17 (Original A12)"},
        {"0xc0e", "Cortex-A17"},
        {"0xc0f", "Cortex-A15"},
        {"0xc14", "Cortex-R4"},
        {"0xc15", "Cortex-R5"},
        {"0xc17", "Cortex-R7"},
        {"0xc18", "Cortex-R8"},
        {"0xc20", "Cortex-M0"},
        {"0xc21", "Cortex-M1"},
        {"0xc23", "Cortex-M3"},
        {"0xc24", "Cortex-M4"},
        {"0xc27", "Cortex-M7"},
        {"0xc60", "Cortex-M0+"},
        {"0xd01", "Cortex-A32"},
        {"0xd03", "Cortex-A53"},
        {"0xd04", "Cortex-A35"},
        {"0xd05", "Cortex-A55"},
        {"0xd07", "Cortex-A57"},
        {"0xd08", "Cortex-A72"},
        {"0xd09", "Cortex-A73"},
        {"0xd0a", "Cortex-A75"},
        {"0xd0b", "Cortex-A76"},
        {"0xd0c", "Neoverse-N1"},
        {"0xd0d", "Cortex-A77"},
        {"0xd13", "Cortex-R52"},
        {"0xd20", "Cortex-M23"},
        {"0xd21", "Cortex-M33"},
        {"0xd40", "Neoverse-V1"},
        {"0xd41", "Cortex-A78"},
        {"0xd42", "Cortex-A78AE"},
        {"0xd44", "Cortex-X1"},
        {"0xd46", "Cortex-A510"},
        {"0xd47", "Cortex-A710"},
        {"0xd48", "Cortex-X2"},
        {"0xd49", "Neoverse-N2"},
        {"0xd4a", "Neoverse-E1"},
        {"0xd4b", "Cortex-A78C"},
        {"0xd4d", "Cortex-A715"}}},
      {"0x42",
       {// Broadcom
        {"0x00f", "Brahma B15"},
        {"0x100", "Brahma B53"},
        {"0x516", "ThunderX2"}}},
      {"0x43",
       {// Cavium
        {"0x0a0", "ThunderX"},
        {"0x0a1", "ThunderX 88XX"},
        {"0x0a2", "ThunderX 81XX"},
        {"0x0a3", "ThunderX 83XX"},
        {"0x0af", "ThunderX2 99xx"}}},
      {"0x44",
       {// DEC
        {"0xa10", "SA110"},
        {"0xa11", "SA1100"}}},
      {"0x4e",
       {// Nvidia
        {"0x000", "Denver"},
        {"0x003", "Denver 2"}}},
      {"0x50",
       {// APM
        {"0x000", "X-Gene"}}},
      {"0x51",
       {// Qualcomm
        {"0x00f", "Scorpion"},
        {"0x02d", "Scorpion"},
        {"0x04d", "Krait"},
        {"0x06f", "Krait"},
        {"0x201", "Kryo"},
        {"0x205", "Kryo"},
        {"0x211", "Kryo"},
        {"0x800", "Falkor V1/Kryo"},
        {"0x801", "Kryo V2"},
        {"0x802", "Kryo 3xx gold"},
        {"0x803", "Kryo 3xx silver"},
        {"0x804", "Kryo 4xx/5xx gold"},
        {"0x805", "Kryo 4xx/5xx silver"},
        {"0xc00", "Falkor"},
        {"0xc01", "Saphira"}}},
      {"0x53",
       {// Samsung
        {"0x001", "Exynos-m1"}}},
      {"0x54",
       {
           // Texas Instruments
           // No models specified
       }},
      {"0x56",
       {// Marvell
        {"0x131", "Feroceon 88FR131"},
        {"0x581", "PJ4/PJ4b"},
        {"0x584", "PJ4B-MP"}}},
      {"0x66",
       {// Faraday
        {"0x526", "FA526"},
        {"0x626", "FA626"}}},
      {"0x69", {// Intel
                {"0x200", "i80200"},        {"0x210", "PXA250A"},
                {"0x212", "PXA210A"},       {"0x242", "i80321-400"},
                {"0x243", "i80321-600"},    {"0x290", "PXA250B/PXA26x"},
                {"0x292", "PXA210B"},       {"0x2c2", "i80321-400-B0"},
                {"0x2c3", "i80321-600-B0"}, {"0x2d0", "PXA250C/PXA255/PXA26x"},
                {"0x2d2", "PXA210C"},       {"0x2e3", "i80219"},
                {"0x411", "PXA27x"},        {"0x41c", "IPX425-533"},
                {"0x41d", "IPX425-400"},    {"0x41f", "IPX425-266"},
                {"0x682", "PXA32x"},        {"0x683", "PXA930/PXA935"},
                {"0x688", "PXA30x"},        {"0x689", "PXA31x"},
                {"0xb11", "SA1110"},        {"0xc12", "IPX1200"}}}};

  if (const auto it = arm_models.find(implementer); it != arm_models.end()) {
    if (const auto part_it = it->second.find(part_hex); part_it != it->second.end()) {
      return part_it->second;
    }
  }
  return "Unknown Model";
}

std::vector<CPU> getAllCPUs() {
  std::vector<CPU> cpus;

  // This map tracks ARM cores based on unique (implementer, variant, part) keys.
  // The int stored is used later to adjust the core counts in a post-processing step.
  std::map<std::tuple<std::string, std::string, std::string>, int> armCoreMap;

  // Open /proc/cpuinfo
  std::ifstream cpuinfo("/proc/cpuinfo");
  if (!cpuinfo.is_open()) {
    return {};  // Could not open
  }

  // Read entire file into a string
  std::string file((std::istreambuf_iterator<char>(cpuinfo)), std::istreambuf_iterator<char>());
  cpuinfo.close();

  // If file content is empty, no data to parse
  if (file.empty()) {
    return {};
  }

  // Split content into blocks, each block typically corresponds to a "processor" section
  auto cpuBlocks = utils::split(file, "\n\n");

  // Remove truly empty blocks (after trimming)
  cpuBlocks.erase(
      std::remove_if(cpuBlocks.begin(), cpuBlocks.end(),
                     [](const std::string& blk) { return blk.find_first_not_of(" \n\r\t") == std::string::npos; }),
      cpuBlocks.end());

  // If there are no blocks, nothing to parse
  if (cpuBlocks.empty()) {
    return {};
  }

  bool isARM = false;         // Will set to true if we detect ARM implementer
  int maxProcessorIndex = 0;  // Tracks how many "processor" entries we've seen
  int cpuId = 0;              // Temporary for the current block's CPU ID

  // Parse each block
  for (const auto& block : cpuBlocks) {
    CPU cpu;

    // Temporary strings used to identify ARM hardware
    std::string implementer;
    std::string partHex;
    std::string variant;

    // Split block into lines
    auto lines = utils::split(block, "\n");
    for (auto& line : lines) {
      // Key-value split by ':'
      auto kv = utils::split(line, ":");
      if (kv.size() < 2) {
        continue;
      }

      std::string name = kv[0];
      std::string value = kv[1];
      utils::strip(name);
      utils::strip(value);

      if (name == "vendor_id") {
        // Intel vendor
        cpu._vendor = value;
      } else if (name == "CPU implementer") {
        // ARM vendor
        implementer = value;
        // Remove "0x" prefix if present
        if (implementer.rfind("0x", 0) == 0) {
          implementer = implementer.substr(2);
        }

        // Check if we know this implementer
        std::string implementerKey = "0x" + implementer;
        auto it = ARM_IMPLEMENTERS.find(implementerKey);
        if (it != ARM_IMPLEMENTERS.end()) {
          isARM = true;
          cpu._vendor = it->second;
        } else {
          cpu._vendor = "Unknown Vendor (" + implementerKey + ")";
        }
      } else if (name == "processor") {
        // The "processor" line should uniquely identify each logical CPU index
        cpuId = std::stoi(value);
        maxProcessorIndex++;  // Count how many logical processors we see
      } else if (name == "model name" || name == "Processor") {
        cpu._modelName = value;
      } else if (name == "cache size") {
        // Intel format: e.g. "4096 KB"
        try {
          auto tokens = utils::split(value, " ");
          cpu._L3CacheSize_Bytes = std::stol(tokens[0]) * 1024;
        } catch (...) {
          // Ignore parse errors
        }
      } else if (name == "siblings") {
        // Intel: number of logical cores
        cpu._numLogicalCores = std::stoi(value);
      } else if (name == "cpu cores") {
        // Intel: number of physical cores
        cpu._numPhysicalCores = std::stoi(value);
      } else if (name == "flags" || name == "Features") {
        // CPU flags/features (Intel "flags", ARM "Features")
        cpu._flags = utils::split(value, " ");
      } else if (name == "physical id") {
        // Intel physical package ID
        cpu._id = std::stoi(value);
      } else if (name == "CPU part") {
        // ARM CPU part (e.g., 0xd03)
        partHex = value;
        if (!implementer.empty()) {
          cpu._modelName = getARMModelName("0x" + implementer, partHex);
        }
      } else if (name == "CPU variant") {
        // ARM CPU variant (e.g., 0x0)
        variant = value;
      }
    }

    // If we detected ARM in at least one block, handle the ARM core map logic
    if (isARM) {
      // Make a unique key for this ARM's implementer + variant + part
      auto armKey = std::make_tuple(implementer, variant, partHex);

      // If we haven't seen this triple, assign new "count" and set ID
      if (armCoreMap.find(armKey) == armCoreMap.end()) {
        // In the original code, this sets maxProcessorIndex to 1, then uses cpuId as the ID.
        // This may or may not match your intended logic, but is preserved from the original.
        maxProcessorIndex = 1;
        cpu._id = cpuId;
      } else {
        // If we already saw this triple, reuse the previous CPU's _id
        // (the code does it by referencing the last CPU in 'cpus')
        if (!cpus.empty()) {
          cpu._id = cpus.back()._id;
        }
      }
      // Update the map. The int stored is `maxProcessorIndex`.
      armCoreMap[armKey] = maxProcessorIndex;
    }

    // Check if we've already inserted a CPU with the same _id (avoid duplicates)
    if (bool isDuplicate = std::any_of(cpus.begin(), cpus.end(), [&](const CPU& c) { return c._id == cpu._id; });
        isDuplicate) {
      // Skip pushing a CPU that has the same _id
      continue;
    }

    // Set clock speeds for this CPU (replace with your actual logic)
    cpu._maxClockSpeed_MHz = getMaxClockSpeed_MHz(cpu._id);
    cpu._regularClockSpeed_MHz = getRegularClockSpeed_MHz(cpu._id);

    // Finally, add this CPU to the list
    cpus.push_back(std::move(cpu));
  }

  // If we detected ARM, adjust physical/logical core counts based on armCoreMap
  if (isARM) {
    // The original code tries to set _numPhysicalCores and _numLogicalCores
    // according to the values stored in armCoreMap. This is simplistic and
    // may not match real hardware topologies.
    std::size_t idx = 0;
    for (const auto& [key, count] : armCoreMap) {
      if (idx < cpus.size()) {
        cpus[idx]._numPhysicalCores = count;
        cpus[idx]._numLogicalCores = count;
        ++idx;
      }
    }
  }

  return cpus;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
