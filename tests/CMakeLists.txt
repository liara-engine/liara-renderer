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

# Creates renderer instances, hence a Vulkan device: `gpu`, which CI excludes. Any warning or error line the renderer
# logs fails the test, validation messages included, even when every CHECK passed.
add_executable(test_renderer_device_cxx test_renderer_device_cxx.cpp)
target_link_libraries(test_renderer_device_cxx PRIVATE
        Liara::Interfaces
        Liara::Renderer
        doctest::doctest)
add_test(NAME test_renderer_device_cxx COMMAND test_renderer_device_cxx)
set_tests_properties(test_renderer_device_cxx PROPERTIES
        LABELS "gpu"
        FAIL_REGULAR_EXPRESSION "\\[liara-renderer\\] (warning|error):")

# The device selection policy alone, without a GPU: the source is compiled into the test, which links no renderer, so
# the same code is tested under every preset with no duplicate symbol and no export needed.
find_package(VulkanHeaders CONFIG REQUIRED)
add_executable(test_renderer_device_selection_cxx
        test_renderer_device_selection_cxx.cpp
        ${LiaraRenderer_SOURCE_DIR}/src/device/DeviceSelection.cpp)
target_include_directories(test_renderer_device_selection_cxx PRIVATE ${LiaraRenderer_SOURCE_DIR}/src)
target_compile_definitions(test_renderer_device_selection_cxx PRIVATE VK_NO_PROTOTYPES)
target_link_libraries(test_renderer_device_selection_cxx PRIVATE
        Vulkan::Headers
        doctest::doctest)
add_test(NAME test_renderer_device_selection_cxx COMMAND test_renderer_device_selection_cxx)
set_tests_properties(test_renderer_device_selection_cxx PROPERTIES LABELS "unit")

# Creation refused for want of a driver: no GPU needed, so `unit`, and it runs in CI, where there is not even a loader.
# The renderer logs an error line on purpose here, so there is no FAIL_REGULAR_EXPRESSION.
add_executable(test_renderer_no_driver_cxx test_renderer_no_driver_cxx.cpp)
target_link_libraries(test_renderer_no_driver_cxx PRIVATE
        Liara::Interfaces
        Liara::Renderer
        doctest::doctest)
add_test(NAME test_renderer_no_driver_cxx COMMAND test_renderer_no_driver_cxx)
set_tests_properties(test_renderer_no_driver_cxx PROPERTIES
        LABELS "unit"
        ENVIRONMENT "VK_DRIVER_FILES=/nonexistent/liara-no-driver.json;VK_ICD_FILENAMES=/nonexistent/liara-no-driver.json")

# Creation without the validation layer: refused in Debug, normal in Release, which needs a GPU.
add_executable(test_renderer_no_layer_cxx test_renderer_no_layer_cxx.cpp)
target_compile_definitions(test_renderer_no_layer_cxx PRIVATE $<$<CONFIG:Debug>:LIARA_DEBUG=1>)
target_link_libraries(test_renderer_no_layer_cxx PRIVATE
        Liara::Interfaces
        Liara::Renderer
        doctest::doctest)
add_test(NAME test_renderer_no_layer_cxx COMMAND test_renderer_no_layer_cxx)
set_tests_properties(test_renderer_no_layer_cxx PROPERTIES
        LABELS "gpu"
        ENVIRONMENT "VK_LOADER_LAYERS_DISABLE=VK_LAYER_KHRONOS_validation")

# The same binary, one case, required to log the GPU it selected: the proof that a device is actually chosen and
# created. With PASS_REGULAR_EXPRESSION set CTest ignores the exit code, which the registration above still checks.
add_test(NAME test_renderer_device_selected_cxx
        COMMAND test_renderer_device_cxx "--test-case=*three times in a row*")
set_tests_properties(test_renderer_device_selected_cxx PROPERTIES
        LABELS "gpu"
        PASS_REGULAR_EXPRESSION "\\[liara-renderer\\] info: selected "
        FAIL_REGULAR_EXPRESSION "\\[liara-renderer\\] (warning|error):")

# The same refusal must say what is missing, so that the person reading it knows what to install: a missing loader on a
# machine without Vulkan (CI), a missing driver here. With PASS_REGULAR_EXPRESSION set CTest ignores the exit code,
# which the registration above still checks.
add_test(NAME test_renderer_no_driver_message_cxx COMMAND test_renderer_no_driver_cxx)
set_tests_properties(test_renderer_no_driver_message_cxx PROPERTIES
        LABELS "unit"
        ENVIRONMENT "VK_DRIVER_FILES=/nonexistent/liara-no-driver.json;VK_ICD_FILENAMES=/nonexistent/liara-no-driver.json"
        PASS_REGULAR_EXPRESSION "\\[liara-renderer\\] error: no Vulkan (loader|driver) found")
