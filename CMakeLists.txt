cmake_minimum_required (VERSION 4.5)

project (Sloptimiser)
enable_language (CXX)

enable_testing()
include (GoogleTest)
add_subdirectory (external)

add_subdirectory (sloptimiser)

