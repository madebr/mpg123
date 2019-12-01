cmake_minimum_required(VERSION 3.12)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libout123/out123.h.in" out123.h)

include_directories("${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libout123/")
add_subdirectory("modules")

set(TARGET libout123)
add_library(${TARGET}
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libout123/libout123.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libout123/stringlists.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libout123/wav.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libout123/hextxt.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libout123/$<$<NOT:$<BOOL:${NO_BUFFER}>>:buffer.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libout123/$<$<NOT:$<BOOL:${NO_BUFFER}>>:xfermem.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libout123/$<$<BOOL:${USE_MODULES}>:module.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libout123/$<$<NOT:$<BOOL:${USE_MODULES}>>:legacy_module.c>")

target_link_libraries(${TARGET} PRIVATE
    compat
    $<TARGET_NAME_IF_EXISTS:defaultmodule>)
target_include_directories(${TARGET} INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

install(TARGETS ${TARGET} EXPORT targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/out123.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
