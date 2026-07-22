# Add competition files to the compilation project
file(GLOB COMPETITION_SRC *.cpp)
add_subdirectory(RangeAnalysisSolver)
set(CHIR_OPT_SRC ${CHIR_OPT_SRC} ${COMPETITION_SRC} PARENT_SCOPE)
