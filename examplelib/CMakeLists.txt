add_library (examplelib STATIC src/core.cpp)
target_include_directories (examplelib PUBLIC include)
target_compile_features (examplelib PRIVATE cxx_std_23)

add_subdirectory (unittest)

