cmake_minimum_required(VERSION 3.12)

project(mpg123 VERSION 1.26.0 LANGUAGES C ASM)

include(cmake/search_libs.cmake)
include(cmake/read_api_version.cmake)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

read_api_version(API_VERSION OUTAPI_VERSION SYNAPI_VERSION)
set(PACKAGE_VERSION ${PROJECT_VERSION})
add_subdirectory("src")

install(
    EXPORT targets
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    NAMESPACE MPG123::)

configure_package_config_file(
    mpg123-config.cmake.in mpg123-config.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
write_basic_package_version_file(
    mpg123-config-version.cmake COMPATIBILITY AnyNewerVersion)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/mpg123-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/mpg123-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "${CMAKE_INSTALL_PREFIX}")
set(libdir "${CMAKE_INSTALL_FULL_LIBDIR}")
set(includedir "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../libmpg123.pc.in" libmpg123.pc @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../libout123.pc.in" libout123.pc @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../libsyn123.pc.in" libsyn123.pc @ONLY)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/libmpg123.pc"
        "${CMAKE_CURRENT_BINARY_DIR}/libout123.pc"
        "${CMAKE_CURRENT_BINARY_DIR}/libsyn123.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../../man1/mpg123.1"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../man1/out123.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}")
