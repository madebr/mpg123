cmake_minimum_required(VERSION 3.12)

option(NO_FEATURE_REPORT "Disable feature report function" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    option(WITH_SSE "x86: build with SSE support if detected. Turn OFF for older devices where autodetection is misleading." ON)
endif()

include_directories("${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/")

if(HAVE_STDLIB_H)
    set(INCLUDE_STDLIB_H "#include <stdlib.h>")
else()
    set(INCLUDE_STDLIB_H "/* #include <stdlib.h> is not available on this system */")
endif()
if(HAVE_SYS_TYPES_H)
    set(INCLUDE_SYS_TYPE_H "#include <sys/types.h>")
else()
    set(INCLUDE_SYS_TYPE_H "/* #include <sys/types.h> is not available on this system */")
endif()



# PPC with AltiVec is missing. But probably obsolete enough for CMake users.

if(ARCH_IS_X64)
    set(MACHINE amd64)
elseif(ARCH_IS_X86)
    set(MACHINE x86)
elseif(ARCH_IS_ARM64)
    set(MACHINE arm64)
elseif(ARCH_IS_ARM32)
    set(MACHINE arm32)
else()
    message(WARNING "Unknown processor. Using generic optimizations.")
    set(MACHINE generic)
endif()
message(STATUS "Detected machine: ${MACHINE}")

set(TARGET lib${PROJECT_NAME})
add_library(${TARGET}
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/parse.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/frame.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/format.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/equalizer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/id3.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/optimize.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/readers.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/tabinit.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/libmpg123.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/index.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<BOOL:${NO_ICY}>>:icy.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<BOOL:${NO_ICY}>>:icy2utf8.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<BOOL:${NO_LAYER1}>>:layer1.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<OR:$<BOOL:${NO_LAYER1}>,$<BOOL:${NO_LAYER2}>>>:layer2.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<BOOL:${NO_LAYER3}>>:layer3.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<BOOL:${NO_NTOM}>>:ntom.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<BOOL:${NO_8BIT}>>:synth_8bit.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<BOOL:${NO_16BIT}>>:synth.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<AND:$<BOOL:${HAVE_FPU}>,$<NOT:$<BOOL:${NO_32BIT}>>>:synth_s32.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<AND:$<BOOL:${HAVE_FPU}>,$<NOT:$<BOOL:${NO_REAL}>>>:synth_real.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<BOOL:${NO_STRING}>>:stringbuf.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<BOOL:${NO_FEATURE_REPORT}>>:feature.c>"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/$<$<NOT:$<BOOL:${PORTABLE_API}>>:lfs_wrap.c>"
    $<TARGET_OBJECTS:compat>)

#if(MSVC AND NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
#    if(MACHINE MATCHES "x86|amd64")
#      find_program(YASM_ASSEMBLER yasm)
#      if(NOT YASM_ASSEMBLER)
#          message(WARNING "Couldn't find yasm assembler for optimizded decoders. Please set YASM_ASSEMBLER variable")
#          set(MACHINE generic)
#      endif()
#    else()
#      message(WARNING "Need work to support non-x86 assembly optimizations with MSVC.")
#      set(MACHINE generic)
#    endif()
#endif()

if(MACHINE STREQUAL "amd64")
    if(HAVE_FPU)
        set(PLATFORM_DEFINITIONS OPT_MULTI OPT_X86_64 OPT_AVX OPT_GENERIC OPT_GENERIC_DITHER)
        set(PLATFORM_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct36_x86_64-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_x86_64_float-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_x86_64_float-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_x86_64_s32-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_x86_64_float-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_x86_64_s32-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct36_avx-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_avx_float-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_avx_float-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_avx_s32-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/getcpuflags_x86_64-intel.S"

#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct36_x86_64.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_x86_64_float.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_x86_64_float.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_x86_64_s32.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_x86_64_float.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_x86_64_s32.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct36_avx.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_avx_float.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_avx_float.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_avx_s32.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/getcpuflags_x86_64.S"
        )
        target_sources(${TARGET} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dither.c")
        if(ACCURATE_ROUNDING)
            list(APPEND PLATFORM_SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_x86_64_accurate-intel.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_x86_64_accurate-intel.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_avx_accurate-intel.S"

#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_x86_64_accurate.S"
#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_x86_64_accurate.S"
#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_avx_accurate.S"
            )
        else()
            list(APPEND PLATFORM_SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_x86_64-intel.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_x86_64-intel.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_x86_64-intel.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_avx-intel.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_avx-intel.S"

#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_x86_64.S"
#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_x86_64.S"
#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_x86_64.S"
#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_avx.S"
#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_avx.S"
            )
        endif()
    else()
        set(PLATFORM_DEFINITIONS OPT_GENERIC)
    endif()

    option(USE_ASSEMBLER "Enable assembler" TRUE)
    include(CMakeDependentOption)
    cmake_dependent_option(USE_NASM "Use NASM assembler" FALSE USE_ASSEMBLER FALSE)
    if(USE_ASSEMBLER)
        set(need_preproc FALSE)
        if(USE_NASM OR CMAKE_C_COMPILER_ID MATCHES "MSVC")
            set(need_preproc TRUE)
        endif()
        set(assembly_sources )
        if(need_preproc)
            set(cc_flags "${CMAKE_C_FLAGS}")
            if(MSVC)
                list(APPEND cc_flags -nologo)
            endif()
            separate_arguments(cc_flags NATIVE_COMMAND)
            # ASMALIGN_BALIGN
            set(asm_definitions ${PLATFORM_DEFINITIONS})
            if(USE_NASM)
                list(APPEND asm_definitions NASM_ASSEMBLER)
            elseif(CMAKE_C_COMPILER MATCHES MSVC)
                list(APPEND asm_definitions MASM_ASSEMBLER)
            endif()
            if(CMAKE_C_COMPILER_ID MATCHES MSVC)
                list(TRANSFORM asm_definitions PREPEND /D)
            else()
                list(TRANSFORM asm_definitions PREPEND -D)
                list(APPEND asm_definitions $<$<BOOL:$<TARGET_PROPERTY:${TARGET},POSITION_INDEPENDENT_CODE>>:-DPIC>)
            endif()
            #            -DASMALIGN_BALIGN
            foreach(platform_source ${PLATFORM_SOURCES})
                get_filename_component(source_name "${platform_source}" NAME)
                set(preprocessed_path "${CMAKE_CURRENT_BINARY_DIR}/${source_name}")
                if(CMAKE_C_COMPILER_ID MATCHES MSVC)
                    set(preproc_command ${CMAKE_C_COMPILER} /nologo ${cc_flags} ${asm_definitions} "/I${PROJECT_SOURCE_DIR}/../../src" "/I${PROJECT_BINARY_DIR}/src" /EP "/Tc${platform_source}" ">${preprocessed_path}" )
                else()
                    set(preproc_command ${CMAKE_C_COMPILER} ${cc_flags} -E -P ${asm_definitions} "-I${PROJECT_SOURCE_DIR}/../../src" "-I${PROJECT_BINARY_DIR}/src" "${platform_source}" -o "${preprocessed_path}")
                endif()
                add_custom_command(OUTPUT "${preprocessed_path}"
                    COMMAND ${preproc_command}
                    MAIN_DEPENDENCY "${platform_source}"
                )
                list(APPEND assembly_sources "${preprocessed_path}")
            endforeach()
        else()
            set(assembly_sources ${PLATFORM_SOURCES})
        endif()
        if(USE_NASM)
            enable_language(ASM_NASM)
            target_sources(${TARGET} PRIVATE ${assembly_sources})
            set_property(SOURCE ${assembly_sources} PROPERTY LANGUAGE ASM_NASM)
        else()
            if(MSVC)
                enable_language(ASM_MASM)
                target_sources(${TARGET} PRIVATE ${assembly_sources})
                set_property(SOURCE ${assembly_sources} PROPERTY LANGUAGE ASM_MASM)
                target_compile_options(${TARGET} PRIVATE $<$<COMPILE_LANGUAGE:ASM_MASM>:-nologo>)
            else()
                enable_language(ASM)
                target_sources(${TARGET} PRIVATE ${assembly_sources})
                set_property(SOURCE ${assembly_sources} PROPERTY LANGUAGE ASM)
            endif()
        endif()
    else()
        # FIXME: how to build mpg123 without any simd code?
        set(PLATFORM_DEFINITIONS OPT_GENERIC)
    endif()
elseif(MACHINE STREQUAL "x86")
    if(TRUE)
        set(PLATFORM_DEFINITIONS OPT_I386)
        target_sources(${TARGET} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_i386.c")
    endif()

    cmake_host_system_information(RESULT HAVE_SSE QUERY HAS_SSE)
    if(CMAKE_SYSTEM_NAME STREQUAL "Android")
        set(HAVE_SSE ${WITH_SSE})
    endif()
    if(HAVE_SSE)
        set(PLATFORM_DEFINITIONS OPT_SSE)
        set(PLATFORM_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/tabinit_mmx-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct36_sse-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_sse_float-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_sse_float-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_sse_float-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_sse_s32-intel.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_sse_s32-intel.S"

#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/tabinit_mmx.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct36_sse.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_sse_float.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_sse_float.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_sse_float.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_sse_s32.S"
#            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_sse_s32.S"
        )
        if(ACCURATE_ROUNDING)
            list(APPEND PLATFORM_SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_sse_accurate-intel.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_sse_accurate-intel.S"

#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_sse_accurate.S"
#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_sse_accurate.S"
            )
        else()
            list(APPEND PLATFORM_SOURCES
#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_sse.S"
#                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_sse.S"

                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_sse-intel.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_sse-intel.S"
            )
        endif()
    endif()

    option(USE_ASSEMBLER "Enable assembler" TRUE)
    include(CMakeDependentOption)
    cmake_dependent_option(USE_NASM "Use NASM assembler" FALSE USE_ASSEMBLER FALSE)
    if(USE_ASSEMBLER)
        set(need_preproc FALSE)
        if(USE_NASM OR CMAKE_C_COMPILER_ID MATCHES "MSVC")
            set(need_preproc TRUE)
        endif()
        set(assembly_sources )
        if(need_preproc)
            set(cc_flags "${CMAKE_C_FLAGS}")
            if(MSVC)
                list(APPEND cc_flags -nologo)
            endif()
            separate_arguments(cc_flags NATIVE_COMMAND)
            # ASMALIGN_BALIGN
            set(asm_definitions ${PLATFORM_DEFINITIONS})
            if(USE_NASM)
                list(APPEND asm_definitions NASM_ASSEMBLER)
            elseif(CMAKE_C_COMPILER MATCHES MSVC)
                list(APPEND asm_definitions MASM_ASSEMBLER)
            endif()
            if(CMAKE_C_COMPILER_ID MATCHES MSVC)
                list(TRANSFORM asm_definitions PREPEND /D)
            else()
                list(TRANSFORM asm_definitions PREPEND -D)
                list(APPEND asm_definitions $<$<BOOL:$<TARGET_PROPERTY:${TARGET},POSITION_INDEPENDENT_CODE>>:-DPIC>)
            endif()
            #            -DASMALIGN_BALIGN
            foreach(platform_source ${PLATFORM_SOURCES})
                get_filename_component(source_name "${platform_source}" NAME)
                set(preprocessed_path "${CMAKE_CURRENT_BINARY_DIR}/${source_name}")
                if(CMAKE_C_COMPILER_ID MATCHES MSVC)
                    set(preproc_command ${CMAKE_C_COMPILER} /nologo ${cc_flags} ${asm_definitions} "/I${PROJECT_SOURCE_DIR}/../../src" "/I${PROJECT_BINARY_DIR}/src" /EP "/Tc${platform_source}" ">${preprocessed_path}" )
                else()
                    set(preproc_command ${CMAKE_C_COMPILER} ${cc_flags} -E -P ${asm_definitions} "-I${PROJECT_SOURCE_DIR}/../../src" "-I${PROJECT_BINARY_DIR}/src" "${platform_source}" -o "${preprocessed_path}")
                endif()
                add_custom_command(OUTPUT "${preprocessed_path}"
                        COMMAND ${preproc_command}
                        MAIN_DEPENDENCY "${platform_source}"
                )
                list(APPEND assembly_sources "${preprocessed_path}")
            endforeach()
        else()
            set(assembly_sources ${PLATFORM_SOURCES})
        endif()
        if(USE_NASM)
            enable_language(ASM_NASM)
            target_sources(${TARGET} PRIVATE ${assembly_sources})
            set_property(SOURCE ${assembly_sources} PROPERTY LANGUAGE ASM_NASM)
        else()
            if(MSVC)
                enable_language(ASM_MASM)
                target_sources(${TARGET} PRIVATE ${assembly_sources})
                set_property(SOURCE ${assembly_sources} PROPERTY LANGUAGE ASM_MASM)
                target_compile_options(${TARGET} PRIVATE $<$<COMPILE_LANGUAGE:ASM_MASM>:-nologo>)
            else()
                enable_language(ASM)
                target_sources(${TARGET} PRIVATE ${assembly_sources})
                set_property(SOURCE ${assembly_sources} PROPERTY LANGUAGE ASM)
            endif()
        endif()
    else()
        # FIXME: how to build mpg123 without any simd code?
        set(PLATFORM_DEFINITIONS OPT_GENERIC)
    endif()
elseif(MACHINE STREQUAL "arm64")
        set(PLATFORM_DEFINITIONS OPT_MULTI OPT_GENERIC OPT_GENERIC_DITHER OPT_NEON64)
        set(PLATFORM_SOURCES
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct36_neon64.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_neon64_float.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_neon64_float.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_neon64_s32.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_neon64_float.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_neon64_s32.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/check_neon.S")
        if(ACCURATE_ROUNDING)
            list(APPEND PLATFORM_SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_neon64_accurate.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_neon64_accurate.S")
        else()
            list(APPEND PLATFORM_SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_neon64.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_neon64.S")
        endif()
        target_sources(${TARGET} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dither.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/getcpuflags_arm.c")
        target_sources(${TARGET} PRIVATE ${PLATFORM_SOURCES})
        if(CMAKE_C_COMPILER_ID MATCHES MSVC)
            enable_language(ASM_MARMASM)
            set_property(SOURCE ${PLATFORM_SOURCES} PROPERTY LANGUAGE ASM_MARMASM)
        else()
            enable_language(ASM)
            set_property(SOURCE ${PLATFORM_SOURCES} PROPERTY LANGUAGE ASM)
        endif()
elseif(MACHINE STREQUAL "arm32")
    if(HAVE_FPU)
            set(PLATFORM_DEFINITIONS OPT_MULTI OPT_GENERIC OPT_GENERIC_DITHER OPT_NEON)
            set(PLATFORM_SOURCES
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct36_neon.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_neon_float.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_neon_float.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_neon_s32.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_neon_float.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_neon_s32.S"
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/check_neon.S")
            target_sources(${TARGET} PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/getcpuflags_arm.c")
            if(ACCURATE_ROUNDING)
                list(APPEND PLATFORM_SOURCES
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_neon_accurate.S"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_neon_accurate.S")
            else()
                list(APPEND PLATFORM_SOURCES
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dct64_neon.S"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_neon.S"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_stereo_neon.S")
            endif()
            target_sources(${TARGET} PRIVATE ${PLATFORM_SOURCES})
            if(CMAKE_C_COMPILER_ID MATCHES MSVC)
                enable_language(ASM_MARMASM)
                set_property(SOURCE ${PLATFORM_SOURCES} PROPERTY LANGUAGE ASM_MARMASM)
            else()
                enable_language(ASM)
                set_property(SOURCE ${PLATFORM_SOURCES} PROPERTY LANGUAGE ASM)
            endif()
            target_sources(${TARGET} PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/dither.c")
    else()
            set(PLATFORM_DEFINITIONS OPT_ARM)
            if(ACCURATE_ROUNDING)
                set(PLATFORM_SOURCES
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_arm_accurate.S")
            else()
                set(PLATFORM_SOURCES
                    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/libmpg123/synth_arm.S")
            endif()
            target_sources(${TARGET} PRIVATE ${PLATFORM_SOURCES})
    endif()
elseif(MACHINE STREQUAL "generic")
    set(PLATFORM_DEFINITIONS OPT_GENERIC)
endif()

read_mpg123_api_version(mpg123_soversion mpg123_version)
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME mpg123)
set_target_properties(${TARGET} PROPERTIES PREFIX lib)
set_target_properties(${TARGET} PROPERTIES SOVERSION ${mpg123_soversion})
set_target_properties(${TARGET} PROPERTIES VERSION ${mpg123_version})
set_target_properties(${TARGET} PROPERTIES DLL_NAME_WITH_SOVERSION TRUE)
set_target_properties(${TARGET} PROPERTIES DEFINE_SYMBOL "")

target_compile_definitions(${TARGET} PRIVATE
    $<$<BOOL:$<TARGET_PROPERTY:POSITION_INDEPENDENT_CODE>>:PIC>)

target_compile_definitions(${TARGET} PRIVATE
    ${PLATFORM_DEFINITIONS}
    $<$<BOOL:${HAVE_FPU}>:REAL_IS_FLOAT>
    $<$<NOT:$<BOOL:${HAVE_FPU}>>:REAL_IS_FIXED>)

if(HAVE_M)
    string(APPEND LIBMPG123_LIBS " -lm")
endif()
if(WANT_WIN32_UNICODE)
    string(APPEND LIBMPG123_LIBS " -lshlwapi")
endif()
set(LIBMPG123_LIBS "${LIBMPG123_LIBS}" PARENT_SCOPE)
target_link_libraries(${TARGET} PRIVATE
    $<$<BOOL:${HAVE_M}>:m>
    $<$<BOOL:${WANT_WIN32_UNICODE}>:shlwapi>)
target_include_directories(${TARGET} INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

install(TARGETS ${TARGET} EXPORT targets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/include/mpg123.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/include/fmt123.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

