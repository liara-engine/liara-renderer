cmake_minimum_required(VERSION 3.29)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
            "In-source builds are not supported. Build into build/<preset-name>/ instead, which is "
            "what the presets already do:\n"
            "    cmake --preset linux-debug-clang\n"
            "Remove the CMakeCache.txt and CMakeFiles/ this attempt just created before retrying.")
endif()

include(GNUInstallDirs)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/.release-please-manifest.json" RELEASE_PLEASE_MANIFEST)
string(JSON PROJECT_VERSION GET ${RELEASE_PLEASE_MANIFEST} ".")
message(STATUS "LiaraRenderer version: ${PROJECT_VERSION}")

project(LiaraRenderer
        VERSION ${PROJECT_VERSION}
        DESCRIPTION "Liara Engine renderer"
        HOMEPAGE_URL "https://github.com/liara-engine/liara-renderer"
        LANGUAGES CXX
)

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

add_library(liara_renderer ${LIARA_RENDERER_SOURCES})
add_library(Liara::Renderer ALIAS liara_renderer)

liara_configure_target(liara_renderer)

target_link_libraries(liara_renderer PUBLIC Liara::Interfaces)

target_link_libraries(liara_renderer PRIVATE Vulkan::Headers GPUOpen::VulkanMemoryAllocator)

# No Vulkan C prototype is declared inside the module, so a direct call to one is a compile error instead of a link-time
# dependency on the loader, which CI machines do not have. Every call goes through Vulkan-Hpp's dispatchers, and VMA
# fetches its functions through the two GetProcAddr pointers it is given.
target_compile_definitions(liara_renderer PRIVATE
        VK_NO_PROTOTYPES
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=1)

# The window-system surface extensions. Defined for the whole target, never per file: these macros change the layout of
# vk::raii's dispatchers, and two files seeing different definitions would disagree on it.
if(WIN32)
    target_compile_definitions(liara_renderer PRIVATE VK_USE_PLATFORM_WIN32_KHR NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # The X11 headers only: the renderer hands the host's Display* to Vulkan and never calls Xlib itself, so it must
    # not link X11::X11. Wayland needs nothing, vulkan_wayland.h declaring the two types it uses.
    find_package(X11 REQUIRED)
    target_compile_definitions(liara_renderer PRIVATE VK_USE_PLATFORM_XLIB_KHR VK_USE_PLATFORM_WAYLAND_KHR)
    target_include_directories(liara_renderer SYSTEM PRIVATE ${X11_INCLUDE_DIR})
endif()

target_include_directories(liara_renderer PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(liara_renderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

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
