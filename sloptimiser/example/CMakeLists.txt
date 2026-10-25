macro(package_add_example EXAMPLE_NAME)
    # create an executable in which the tests will be stored
    add_executable(${EXAMPLE_NAME} ${ARGN})
    target_link_libraries(${EXAMPLE_NAME} sloptimiser)
    target_compile_features (${EXAMPLE_NAME} PRIVATE cxx_std_23)
    set_target_properties(${EXAMPLE_NAME} PROPERTIES FOLDER example)
endmacro()