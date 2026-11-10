add_library (sloptimiser INTERFACE)
target_include_directories (sloptimiser INTERFACE include)
target_link_libraries (sloptimiser INTERFACE BLAS::BLAS)

add_subdirectory (unittest)
add_subdirectory (example)

