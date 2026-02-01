#include "clad/Differentiator/Tape.h"
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include "benchmark/benchmark.h"
extern "C" {
void pushReal8(double x);
void popReal8(double* x);
}

static void BM_Tapenade_PushPop(benchmark::State& state) {
  int n = state.range(0);

  for (auto _ : state) {
    // 1. Push Loop
    for (int i = 0; i < n; ++i) {
      double val = 1.0;
      pushReal8(val);
    }

    // 2. Pop Loop
    for (int i = 0; i < n; ++i) {
      double val;
      popReal8(&val);
      benchmark::DoNotOptimize(val);
    }
  }
}

static void BM_Clad_Disk_PushPop(benchmark::State& state) {
  int n = state.range(0);

  clad::tape_impl<double, 64, 1024, false, true> t;

  for (auto _ : state) {
    for (int i = 0; i < n; ++i)
      t.emplace_back(1.0);

    for (int i = 0; i < n; ++i) {
      benchmark::DoNotOptimize(t.back());
      t.pop_back();
    }
  }
}

BENCHMARK(BM_Tapenade_PushPop)
    ->RangeMultiplier(4)
    ->Range(1024, 262144)
    ->Name("Tapenade_Stack");

BENCHMARK(BM_Clad_Disk_PushPop)
    ->RangeMultiplier(4)
    ->Range(1024, 262144)
    ->Name("Clad_Disk_Stack");

BENCHMARK_MAIN();