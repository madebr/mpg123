function(read_project_version project_version)

    file( READ "${MPG123_SOURCE_ROOT}/src/version.h" version_h )

    string( REGEX MATCH "#define +MPG123_MAJOR +([0-9]+)" result ${version_h} )
    set( major_version ${CMAKE_MATCH_1})
    string( REGEX MATCH "#define +MPG123_MINOR +([0-9]+)" result ${version_h} )
    set( minor_version ${CMAKE_MATCH_1})
    string( REGEX MATCH "#define +MPG123_PATCH +([0-9]+)" result ${version_h} )
    set( patch_version ${CMAKE_MATCH_1})

#    string( REGEX MATCH "#define +MPG123_SUFFIX +\"([^\"]+)\"" result ${version_h} )
#    set( version_suffix ${CMAKE_MATCH_1})
# CMake project() chokes on version with suffix, so give it just the numbers.
    set( ${project_version} ${major_version}.${minor_version}.${patch_version} PARENT_SCOPE)

endfunction()

function(read_mpg123_api_version soversion version)
    file( READ "${MPG123_SOURCE_ROOT}/src/include/mpg123.h" mpg123_h )

    string( REGEX MATCH "#define +MPG123_API_VERSION +([0-9]+)" result ${mpg123_h} )
    set( api_version ${CMAKE_MATCH_1})
    string( REGEX MATCH "#define +MPG123_PATCHLEVEL +([0-9]+)" result ${mpg123_h} )
    set( patch_level ${CMAKE_MATCH_1})

    set(${soversion} "0" PARENT_SCOPE)
    set(${version} "0.${api_version}.${patch_level}" PARENT_SCOPE)
endfunction()

function(read_out123_api_version soversion version)
    file( READ "${MPG123_SOURCE_ROOT}/src/include/out123.h" out123_h )

    string( REGEX MATCH "#define +OUT123_API_VERSION +([0-9]+)" result ${out123_h} )
    set( api_version ${CMAKE_MATCH_1})
    string( REGEX MATCH "#define +OUT123_PATCHLEVEL +([0-9]+)" result ${out123_h} )
    set( patch_level ${CMAKE_MATCH_1})

    set(${soversion} "0" PARENT_SCOPE)
    set(${version} "0.${api_version}.${patch_level}" PARENT_SCOPE)
endfunction()

function(read_syn123_api_version soversion version)
    file( READ "${MPG123_SOURCE_ROOT}/src/include/syn123.h" syn123_h )

    string( REGEX MATCH "#define +SYN123_API_VERSION +([0-9]+)" result ${syn123_h} )
    set( api_version ${CMAKE_MATCH_1})
    string( REGEX MATCH "#define +SYN123_PATCHLEVEL +([0-9]+)" result ${syn123_h} )
    set( patch_level ${CMAKE_MATCH_1})

    set(${soversion} "0" PARENT_SCOPE)
    set(${version} "0.${api_version}.${patch_level}" PARENT_SCOPE)
endfunction()
