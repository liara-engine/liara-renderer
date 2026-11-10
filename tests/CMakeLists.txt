find_package(doctest CONFIG REQUIRED)

add_executable(test_renderer_functions_cxx test_renderer_cxx.cpp)
target_link_libraries(test_renderer_functions_cxx PRIVATE
        Liara::Interfaces
        Liara::Renderer
        doctest::doctest)
add_test(NAME test_renderer_functions_cxx COMMAND test_renderer_functions_cxx)
set_tests_properties(test_renderer_functions_cxx PROPERTIES LABELS "unit")



# The console renderer is private and header-only: the test includes it from src/ and links no renderer, so it needs
# no instance and no GPU.
add_executable(test_renderer_console_cxx test_renderer_console_cxx.cpp)
target_include_directories(test_renderer_console_cxx PRIVATE ${LiaraRenderer_SOURCE_DIR}/src)
target_link_libraries(test_renderer_console_cxx PRIVATE
        Liara::Interfaces
        doctest::doctest)
add_test(NAME test_renderer_console_cxx COMMAND test_renderer_console_cxx)
set_tests_properties(test_renderer_console_cxx PROPERTIES LABELS "unit")
