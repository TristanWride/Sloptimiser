cmake_minimum_required (VERSION 3.27)

project (CMakeTemplate)


if(MSVC)
  # Force to always compile with W4
  add_compile_options (/W4 /EHsc "$<$<CONFIG:Debug>:/Od>" "$<$<CONFIG:Release>:/O2>")
else ()
  # Update if necessary
  add_compile_options (-Wall -pedantic)
endif ()

enable_testing()
include (GoogleTest)
add_subdirectory (external)

add_subdirectory (sloptimiser)


add_executable (main src/main.cpp)
target_compile_features (main PUBLIC cxx_std_23)
target_include_directories (main PUBLIC include)
target_link_libraries (main PUBLIC sloptimiser)

