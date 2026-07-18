cmake_minimum_required(VERSION 3.29)

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
        "${CMAKE_CURRENT_BINARY_DIR}/generated/config.h"
        @ONLY)

if(NOT TARGET Liara::Interfaces)
    message(FATAL_ERROR
            "Liara::Interfaces is not available. Build the renderer through the "
            "workspace superbuild (see scripts/setup-workspace.sh).")
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

file(GLOB_RECURSE LIARA_RENDERER_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.inl"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.ipp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.tpp"
)

add_library(liara_renderer STATIC ${LIARA_RENDERER_SOURCES})
add_library(Liara::Renderer ALIAS liara_renderer)

target_link_libraries(liara_renderer PRIVATE Liara::Interfaces)

target_include_directories(liara_renderer PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(LIARA_RENDERER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(LIARA_RENDERER_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(DIRECTORY include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING
            PATTERN "*.h"
            PATTERN "config.h.in" EXCLUDE
    )

    install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/config.h"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/liara/renderer"
    )

    install(TARGETS liara_renderer
            EXPORT LiaraRendererTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT LiaraRendererTargets
            FILE LiaraRendererTargets.cmake
            NAMESPACE Liara::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LiaraRenderer
    )

    configure_package_config_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LiaraRendererConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/LiaraRendererConfig.cmake"
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LiaraRenderer
    )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/LiaraRendererConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY AnyNewerVersion
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/LiaraRendererConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/LiaraRendererConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LiaraRenderer
    )
endif()