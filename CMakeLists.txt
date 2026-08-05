cmake_minimum_required(VERSION 3.29)

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
            " (see https://liara-engine.liara-engine-documentation.workers.dev/liara/latest/book/BOOTSTRAP)")
endif()

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

target_link_libraries(liara_renderer PUBLIC Liara::Interfaces)

target_include_directories(liara_renderer PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(liara_renderer PRIVATE LIARA_RENDERER_BUILD)

set_target_properties(liara_renderer PROPERTIES
        EXPORT_NAME Renderer
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        POSITION_INDEPENDENT_CODE ON
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${PROJECT_VERSION}
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(liara_renderer PUBLIC LIARA_RENDERER_SHARED)
else()
    target_compile_definitions(liara_renderer PUBLIC LIARA_RENDERER_STATIC)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(liara_renderer PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
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

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/liara/renderer"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/liara"
            COMPONENT liara-renderer-dev
            FILES_MATCHING PATTERN "*.h"
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
