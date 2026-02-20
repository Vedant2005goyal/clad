// RUN: %cladclang -std=c++17 -fno-exceptions -I%S/../../include %s -o %t
// RUN: %t | %filecheck_exec %s

#include "clad/Differentiator/Tape.h"
#include <cstdio>
#include <string>

struct MockTape {
  float last_val = 0.0f;
  void emplace_back(float val) { 
    last_val = val;
    printf("Mock push: %.2f\n", val); 
  }
  float back() const { return 99.9f; }
  void pop_back() {}
};

namespace clad {
  template <>
  struct TapeTraits<float, 64, 1024, false, true> {
    using Type = MockTape; 
  };
}

int main() {
  // Test standard tape behavior
  clad::custom_tape<double, 64, 1024, false, true> default_tape;
  default_tape.emplace_back(3.14);
  printf("Standard tape back: %.2f\n", default_tape.back());
  // CHECK-EXEC: Standard tape back: 3.14

  // Test RamDiskManager behavior
  clad::detail::RamDiskManager<double, 1024> custom_manager("/custom/ram/disk/");
  bool has_path = custom_manager.filename.find("/custom/ram/disk/") == 0;
  printf("Custom RamDisk path ok: %d\n", has_path);
  // CHECK-EXEC: Custom RamDisk path ok: 1

  // Test Tape Specialization Hook
  // It will use MockTape rather tape_impl since we have used float instead of double here
  clad::custom_tape<float, 64, 1024, false, true> specialized_tape;
  specialized_tape.emplace_back(2.71f);
  // CHECK-EXEC: Mock push: 2.71
  printf("Specialized tape back: %.1f\n", specialized_tape.back());
  // CHECK-EXEC: Specialized tape back: 99.9

  return 0;
}