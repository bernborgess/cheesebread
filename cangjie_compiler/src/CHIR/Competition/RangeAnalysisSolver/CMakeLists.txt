cmake_minimum_required(VERSION 3.14)
project(RangeAnalysisSolver VERSION 1.0 LANGUAGES CXX)

# ==========================================
# 1. Context Detection
# ==========================================
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(RANGE_ANALYSIS_STANDALONE TRUE)
else()
    set(RANGE_ANALYSIS_STANDALONE FALSE)
endif()

# ==========================================
# 2. Core Sources
# ==========================================
set(SOLVER_CORE_SOURCES
    src/Constraint.cpp
    src/Solver.cpp
    src/Graph.cpp
)

# ==========================================
# 3. Standalone Mode (Isolated Build)
# ==========================================
if(RANGE_ANALYSIS_STANDALONE)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Main Application Target
    add_executable(lattice_analyzer
        src/main.cpp
        ${SOLVER_CORE_SOURCES}
    )
   target_include_directories(lattice_analyzer PRIVATE ../../../../include/cangjie/Competition/RangeAnalysisSolver)

    # Testing Target (Using Catch2 via FetchContent)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(analyzer_tests
        tests/test_abstract_value.cpp
        tests/test_constraints.cpp
        tests/test_solver.cpp
        ${SOLVER_CORE_SOURCES}
    )

    target_include_directories(analyzer_tests PRIVATE ../../../../include/cangjie/Competition/RangeAnalysisSolver)
    target_link_libraries(analyzer_tests PRIVATE Catch2::Catch2WithMain)
    
    enable_testing()
    add_test(NAME LatticeTests COMMAND analyzer_tests)

# ==========================================
# 4. Integrated Mode (Cangjie Compiler Build)
# ==========================================
else()
    # Must match exactly with $<TARGET_OBJECTS:RangeAnalysisSolver>
    add_library(RangeAnalysisSolver OBJECT ${SOLVER_CORE_SOURCES})
    
    target_include_directories(RangeAnalysisSolver PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/cangjie/Competition/RangeAnalysisSolver>
    )
    
    if(DEFINED CJC_EXTRA_WARNINGS)
        target_compile_options(RangeAnalysisSolver PRIVATE "${CJC_EXTRA_WARNINGS}")
    endif()
endif()


