set(CTEST_BUILD_NAME ${ROOT_ARCHITECTURE}-${CMAKE_BUILD_TYPE})
enable_testing()

CB_ADD_GBENCHMARK(Simple Simple.cpp)
CB_ADD_GBENCHMARK(AlgorithmicComplexity AlgorithmicComplexity.cpp)
CB_ADD_GBENCHMARK(ArrayExpressionTemplates ArrayExpressionTemplates.cpp)
if (CLAD_ENABLE_ENZYME_BACKEND)
  CB_ADD_GBENCHMARK(EnzymeCladComparison EnzymeCladComparison.cpp)
endif(CLAD_ENABLE_ENZYME_BACKEND)

option(CLAD_ENABLE_TAPENADE_BENCHMARK "Enable Tapenade comparison benchmarks" OFF)

if (CLAD_ENABLE_TAPENADE_BENCHMARK)
  include(FetchContent)
  FetchContent_Declare(
    tapenade_kit
    GIT_REPOSITORY https://gitlab.inria.fr/tapenade/tapenade.git
    GIT_TAG        develop
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE      
    GIT_SUBMODULES ""
  )
  FetchContent_MakeAvailable(tapenade_kit)

  add_library(tapenade_support STATIC ${tapenade_kit_SOURCE_DIR}/ADFirstAidKit/adStack.c)
  target_include_directories(tapenade_support PUBLIC ${tapenade_kit_SOURCE_DIR}/ADFirstAidKit)

  target_compile_definitions(tapenade_support PRIVATE 
      ADSTACK_MAX_SPACES=1        
      ADSTACK_BLOCK_SIZE=4096    
  )
  CB_ADD_GBENCHMARK(TapenadeVsClad TapenadeVsClad.cpp)
  
  target_link_libraries(TapenadeVsClad PRIVATE tapenade_support)
endif()

CB_ADD_GBENCHMARK(VectorModeComparison VectorModeComparison.cpp)
CB_ADD_GBENCHMARK(MemoryComplexity MemoryComplexity.cpp)
CB_ADD_GBENCHMARK(Multithreading Multithreading.cpp)
CB_ADD_GBENCHMARK(Hessians Hessians.cpp)

set (CLAD_BENCHMARK_DEPS clad)
get_property(_benchmark_names DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY TESTS)

foreach (name ${_benchmark_names})
  get_test_property(${name} LABELS _labels)
  if (_labels MATCHES ".*benchmark.*")
    get_test_property(${name} DEPENDS _deps)
    list(APPEND CLAD_BENCHMARK_DEPS ${_deps})
  endif()
endforeach()

add_custom_target(benchmark-clad COMMAND ${CMAKE_CTEST_COMMAND} -V
  DEPENDS ${CLAD_BENCHMARK_DEPS} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(benchmark-clad PROPERTIES FOLDER "Clad benchmarks")