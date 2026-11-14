find_package(doctest CONFIG REQUIRED)

add_executable(test_renderer_functions_cxx test_renderer_cxx.cpp)
target_include_directories(test_renderer_functions_cxx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_renderer_functions_cxx PRIVATE
        Liara::Interfaces
        Liara::Renderer
        doctest::doctest)
add_test(NAME test_renderer_functions_cxx COMMAND test_renderer_functions_cxx)
# No argument failure may reach Vulkan, and opening the loader always logs a line: any renderer line fails the test.
set_tests_properties(test_renderer_functions_cxx PROPERTIES
        LABELS "unit"
        FAIL_REGULAR_EXPRESSION "\\[liara-renderer\\] ")



# The console renderer is private and header-only: the test includes it from src/ and links no renderer, so it needs
# no instance and no GPU.
add_executable(test_renderer_console_cxx
        test_renderer_console_cxx.cpp
        ${LiaraRenderer_SOURCE_DIR}/src/Log.cpp)
target_include_directories(test_renderer_console_cxx PRIVATE ${LiaraRenderer_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_renderer_console_cxx PRIVATE
        Liara::Interfaces
        doctest::doctest)
add_test(NAME test_renderer_console_cxx COMMAND test_renderer_console_cxx)
set_tests_properties(test_renderer_console_cxx PROPERTIES LABELS "unit")

# The log sink alone, compiled into the test like the console renderer above: no instance, no GPU.
add_executable(test_renderer_log_cxx
        test_renderer_log_cxx.cpp
        ${LiaraRenderer_SOURCE_DIR}/src/Log.cpp)
target_include_directories(test_renderer_log_cxx PRIVATE ${LiaraRenderer_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_renderer_log_cxx PRIVATE doctest::doctest)
add_test(NAME test_renderer_log_cxx COMMAND test_renderer_log_cxx)
set_tests_properties(test_renderer_log_cxx PROPERTIES LABELS "unit")

# Creates renderer instances, hence a Vulkan device: `gpu`, which CI excludes. Any warning or error line the renderer
# logs fails the test, validation messages included, even when every CHECK passed.
add_executable(test_renderer_device_cxx test_renderer_device_cxx.cpp)
target_include_directories(test_renderer_device_cxx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_renderer_device_cxx PRIVATE
        Liara::Interfaces
        Liara::Renderer
        doctest::doctest)
add_test(NAME test_renderer_device_cxx COMMAND test_renderer_device_cxx "--test-case-exclude=*no device matches*")
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

# The swapchain's choices alone, compiled in like the selection policy above.
add_executable(test_renderer_swapchain_choices_cxx
        test_renderer_swapchain_choices_cxx.cpp
        ${LiaraRenderer_SOURCE_DIR}/src/swapchain/SwapchainChoices.cpp)
target_include_directories(test_renderer_swapchain_choices_cxx PRIVATE ${LiaraRenderer_SOURCE_DIR}/src)
target_compile_definitions(test_renderer_swapchain_choices_cxx PRIVATE VK_NO_PROTOTYPES)
target_link_libraries(test_renderer_swapchain_choices_cxx PRIVATE
        Liara::Interfaces
        Vulkan::Headers
        doctest::doctest)
add_test(NAME test_renderer_swapchain_choices_cxx COMMAND test_renderer_swapchain_choices_cxx)
set_tests_properties(test_renderer_swapchain_choices_cxx PROPERTIES LABELS "unit")

# Creation refused for want of a driver: no GPU needed, so `unit`, and it runs in CI, where there is not even a loader.
# The renderer logs an error line on purpose here, so there is no FAIL_REGULAR_EXPRESSION.
add_executable(test_renderer_no_driver_cxx test_renderer_no_driver_cxx.cpp)
target_include_directories(test_renderer_no_driver_cxx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
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
target_include_directories(test_renderer_no_layer_cxx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
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

# The same binary, one case: a preferred GPU no device matches. Creation goes on with the usual choice, and says why in
# a warning, which is what this registration requires and what the main one would fail on.
add_test(NAME test_renderer_device_preference_cxx COMMAND test_renderer_device_cxx "--test-case=*no device matches*")
set_tests_properties(test_renderer_device_preference_cxx PROPERTIES
        LABELS "gpu"
        PASS_REGULAR_EXPRESSION "\\[liara-renderer\\] warning: no GPU name contains \"liara-no-such-gpu\"")

# The same refusal must say what is missing, so that the person reading it knows what to install: a missing loader on a
# machine without Vulkan (CI), a missing driver here. With PASS_REGULAR_EXPRESSION set CTest ignores the exit code,
# which the registration above still checks.
add_test(NAME test_renderer_no_driver_message_cxx COMMAND test_renderer_no_driver_cxx)
set_tests_properties(test_renderer_no_driver_message_cxx PROPERTIES
        LABELS "unit"
        ENVIRONMENT "VK_DRIVER_FILES=/nonexistent/liara-no-driver.json;VK_ICD_FILENAMES=/nonexistent/liara-no-driver.json"
        PASS_REGULAR_EXPRESSION "\\[liara-renderer\\] error: no Vulkan (loader|driver) found")
