cmake_minimum_required(VERSION 3.29)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
            "In-source builds are not supported. Build into build/<preset-name>/ instead, which is "
            "what the presets already do:\n"
            "    cmake --preset linux-debug-clang\n"
            "Remove the CMakeCache.txt and CMakeFiles/ this attempt just created before retrying.")
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/.release-please-manifest.json" RELEASE_PLEASE_MANIFEST)
string(JSON PROJECT_VERSION GET ${RELEASE_PLEASE_MANIFEST} ".")
message(STATUS "LiaraRenderer version: ${PROJECT_VERSION}")

project(LiaraRenderer
        VERSION ${PROJECT_VERSION}
        DESCRIPTION "Liara Engine renderer"
        HOMEPAGE_URL "https://github.com/liara-engine/liara-renderer"
        LANGUAGES CXX
)
include(GNUInstallDirs)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/liara/renderer/config.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/generated/liara/renderer/config.h"
        @ONLY)

if(NOT TARGET Liara::Interfaces)
    find_package(LiaraInterfaces CONFIG QUIET)
endif()

if(NOT TARGET Liara::Interfaces)
    message(FATAL_ERROR
            "Liara::Interfaces is not available. Either build liara-renderer through the workspace superbuild, "
            "or install liara-interfaces and point find_package(LiaraInterfaces) at it via CMAKE_PREFIX_PATH "
            " (see https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/guides/bootstrap)")
endif()

# Header-only, both of them: Vulkan-Hpp comes with vulkan-headers, and the loader is opened at run time rather than
# linked (VK_NO_PROTOTYPES below; ADR 0016).
find_package(VulkanHeaders CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# The pinned glslc, built by vcpkg from the shaderc host dependency (ADR 0017). Never a glslc from the PATH, whose
# version nobody chose: the shaders are compiled by the same compiler on every machine and every CI leg. The presets
# leave VCPKG_HOST_TRIPLET to vcpkg's default, which CMake then does not see, so the installed triplets are searched.
if(VCPKG_HOST_TRIPLET)
    set(LIARA_GLSLC_DIRS "${VCPKG_INSTALLED_DIR}/${VCPKG_HOST_TRIPLET}/tools/shaderc")
else()
    file(GLOB LIARA_GLSLC_DIRS LIST_DIRECTORIES true "${VCPKG_INSTALLED_DIR}/*/tools/shaderc")
endif()
find_program(LIARA_GLSLC glslc PATHS ${LIARA_GLSLC_DIRS} NO_DEFAULT_PATH)
if(NOT LIARA_GLSLC)
    message(FATAL_ERROR "glslc was not found in vcpkg's host tools (${VCPKG_INSTALLED_DIR}/<host triplet>/tools/"
                        "shaderc). It comes from the shaderc port in vcpkg.json; re-run ./scripts/liara.sh setup.")
endif()
message(STATUS "LiaraRenderer glslc: ${LIARA_GLSLC}")

# Each shader becomes generated/shaders/<name>.inc, glslc's `-mfmt=c` output: the SPIR-V words as a braced initializer
# list, which src/pipelines/Shaders.cpp includes into an array. Debug keeps the source for RenderDoc.
set(LIARA_RENDERER_SHADERS mesh.vert mesh.frag)
set(LIARA_RENDERER_SHADER_OUTPUTS "")
foreach(shader IN LISTS LIARA_RENDERER_SHADERS)
    set(shader_source "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}")
    set(shader_output "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders/${shader}.inc")
    add_custom_command(
            OUTPUT "${shader_output}"
            COMMAND "${LIARA_GLSLC}" --target-env=vulkan1.3 -Werror -mfmt=c $<IF:$<CONFIG:Debug>,-g,-O>
                    -o "${shader_output}" "${shader_source}"
            DEPENDS "${shader_source}"
            COMMENT "Compiling shader ${shader}"
            VERBATIM)
    list(APPEND LIARA_RENDERER_SHADER_OUTPUTS "${shader_output}")
endforeach()
add_custom_target(liara_renderer_shaders DEPENDS ${LIARA_RENDERER_SHADER_OUTPUTS})

if(PROJECT_IS_TOP_LEVEL)
    set(LIARA_RENDERER_IS_TOP_LEVEL ON)
else()
    set(LIARA_RENDERER_IS_TOP_LEVEL OFF)
endif()

option(LIARA_RENDERER_BUILD_TESTS
        "Build the liara-renderer test suite."
        ${LIARA_RENDERER_IS_TOP_LEVEL})
option(LIARA_RENDERER_INSTALL
        "Generate install targets for liara-renderer."
        ${LIARA_RENDERER_IS_TOP_LEVEL})

if(LIARA_RENDERER_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

file(GLOB_RECURSE LIARA_RENDERER_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
list(REMOVE_ITEM LIARA_RENDERER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/renderer.cpp")

# Everything behind the ABI shim, compiled once. The library is the shim plus these objects, and the tests that need the
# renderer's internals link them directly, so they exercise the very code the module ships, under every preset, shared
# ones included, with nothing exported for them. The target is never installed, so its usage requirements may be
# PUBLIC: they reach this module's own targets and nothing else.
add_library(liara_renderer_objects OBJECT ${LIARA_RENDERER_SOURCES})
liara_configure_target(liara_renderer_objects)
add_dependencies(liara_renderer_objects liara_renderer_shaders)

target_link_libraries(liara_renderer_objects PUBLIC Liara::Interfaces Vulkan::Headers GPUOpen::VulkanMemoryAllocator)

# No Vulkan C prototype is declared inside the module, so a direct call to one is a compile error instead of a link-time
# dependency on the loader, which CI machines do not have. Every call goes through Vulkan-Hpp's dispatchers, and VMA
# fetches its functions through the two GetProcAddr pointers it is given.
target_compile_definitions(liara_renderer_objects PUBLIC
        VK_NO_PROTOTYPES
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=1)

# The window-system surface extensions. Defined for every file that sees Vulkan-Hpp, never per file: these macros change
# the layout of vk::raii's dispatchers, and two files seeing different definitions would disagree on it. PUBLIC for that
# reason: the shim and the tests linking the objects compile with the same ones.
if(WIN32)
    target_compile_definitions(liara_renderer_objects PUBLIC VK_USE_PLATFORM_WIN32_KHR NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # The X11 headers only: the renderer hands the host's Display* to Vulkan and never calls Xlib itself, so it must
    # not link X11::X11. Wayland needs nothing, vulkan_wayland.h declaring the two types it uses.
    find_package(X11 REQUIRED)
    target_compile_definitions(liara_renderer_objects PUBLIC VK_USE_PLATFORM_XLIB_KHR VK_USE_PLATFORM_WAYLAND_KHR)
    target_include_directories(liara_renderer_objects SYSTEM PUBLIC ${X11_INCLUDE_DIR})
endif()

target_include_directories(liara_renderer_objects PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/generated
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(liara_renderer "${CMAKE_CURRENT_SOURCE_DIR}/src/renderer.cpp")
add_library(Liara::Renderer ALIAS liara_renderer)

liara_configure_target(liara_renderer)

target_link_libraries(liara_renderer PUBLIC Liara::Interfaces)

# The objects, and their usage requirements. Wrapped so that an installed export never names a target it does not
# install; the dependencies the objects bring are named again for that export.
target_link_libraries(liara_renderer PRIVATE
        $<BUILD_INTERFACE:liara_renderer_objects>
        Vulkan::Headers
        GPUOpen::VulkanMemoryAllocator)

target_include_directories(liara_renderer PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(liara_renderer PRIVATE LIARA_RENDERER_BUILD)

set_target_properties(liara_renderer PROPERTIES
        EXPORT_NAME Renderer
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${PROJECT_VERSION}
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(liara_renderer PUBLIC LIARA_RENDERER_SHARED)
else()
    target_compile_definitions(liara_renderer PUBLIC LIARA_RENDERER_STATIC)
endif()

if(LIARA_RENDERER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(LIARA_RENDERER_INSTALL)
    include(CMakePackageConfigHelpers)

    install(TARGETS liara_renderer
            EXPORT LiaraRendererTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT liara-renderer-runtime
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT liara-renderer-runtime
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT liara-renderer-dev
    )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/liara/renderer/config.h"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/liara/renderer"
            COMPONENT liara-renderer-dev
    )

    install(EXPORT LiaraRendererTargets
            FILE LiaraRendererTargets.cmake
            NAMESPACE Liara::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LiaraRenderer
            COMPONENT liara-renderer-dev
    )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/LiaraRendererConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LiaraRendererConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/LiaraRendererConfig.cmake"
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LiaraRenderer
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/LiaraRendererConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/LiaraRendererConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LiaraRenderer
            COMPONENT liara-renderer-dev
    )
endif()
