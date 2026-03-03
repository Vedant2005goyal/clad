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

  // Test RamDiskManager 
  clad::detail::RamDiskManager<double, 1024> mmap_manager;
  double test_data[1024];
  for(int i = 0; i < 1024; ++i) 
    test_data[i] = i * 1.5;

  size_t offset = mmap_manager.write_slab(test_data);
  double read_data[1024];
  mmap_manager.read_slab(read_data, offset);

  bool mmap_ok = (read_data[0] == 0.0 && read_data[1023] == 1023 * 1.5);
  printf("Mmap manager read/write ok: %d\n", mmap_ok);
  // CHECK-EXEC: Mmap manager read/write ok: 1

  // Test Tape Specialization Hook
  // It will use MockTape rather tape_impl since we have used float instead of double here
  clad::custom_tape<float, 64, 1024, false, true> specialized_tape;
  specialized_tape.emplace_back(2.71f);
  // CHECK-EXEC: Mock push: 2.71
  printf("Specialized tape back: %.1f\n", specialized_tape.back());
  // CHECK-EXEC: Specialized tape back: 99.9

  clad::detail::DiskManager<double, 1024> file_backed_manager("/tmp");
  
  size_t file_offset = file_backed_manager.write_slab(test_data);
  double file_read_data[1024];
  file_backed_manager.read_slab(file_read_data, file_offset);

  bool file_mmap_ok = (file_read_data[0] == 0.0 && file_read_data[1023] == 1023 * 1.5);
  printf("File-Backed manager read/write ok: %d\n", file_mmap_ok);
  // CHECK-EXEC: File-Backed manager read/write ok: 1

  return 0;
}