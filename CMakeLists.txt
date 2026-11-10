cmake_minimum_required (VERSION 4.4.2)

project (Sloptimiser)
enable_language (CXX)

enable_testing()
include (GoogleTest)
find_package (BLAS REQUIRED)

add_subdirectory (external)

add_subdirectory (sloptimiser)

