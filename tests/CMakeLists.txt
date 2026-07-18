find_package(doctest CONFIG REQUIRED)

add_executable(test_renderer_functions_cxx test_renderer_cxx.cpp)
target_link_libraries(test_renderer_functions_cxx PRIVATE
        Liara::Interfaces
        Liara::Renderer
        doctest::doctest)
add_test(NAME test_renderer_functions_cxx COMMAND test_renderer_functions_cxx)


