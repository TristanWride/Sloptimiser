cmake_minimum_required (VERSION 3.27)

project (CMakeTemplate)
enable_language (CXX)

enable_testing()
include (GoogleTest)
add_subdirectory (external)

add_subdirectory (examplelib)

add_executable (main src/main.cpp)
target_compile_features (main PUBLIC cxx_std_23)
target_include_directories (main PUBLIC include)
target_link_libraries (main PUBLIC examplelib)

