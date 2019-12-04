set(TARGET compat)
add_library(${TARGET} STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/compat/compat.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/compat/compat_str.c")
set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${TARGET} PRIVATE
    $<$<BOOL:${HAVE_M}>:m>)

add_library(${TARGET}_dl STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/compat/compat_dl.c")
set_target_properties(${TARGET}_dl PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${TARGET}_dl PRIVATE
    $<$<BOOL:${HAVE_UNIX_DL}>:dl>)

add_library(${TARGET}_str STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/compat/compat_str.c")
set_target_properties(${TARGET}_dl PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(NOT UNIX OR NOT BUILD_SHARED_LIBS)
    install(TARGETS ${TARGET} ${TARGET}_str EXPORT targets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/")
endif()
