add_library (sloptimiser INTERFACE)
target_include_directories (sloptimiser INTERFACE include)
# target_compile_features (examplelib PRIVATE cxx_std_23)

add_subdirectory (unittest)
add_subdirectory (example)

