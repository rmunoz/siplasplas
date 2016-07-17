if(WIN32)
    return()
endif()

include(libclang)

libclang_system_include_dir(LIBCLANG_SYSTEM_INCLUDE_DIR)

add_siplasplas_thirdparty(standardese
GIT_REPOSITORY
    "https://github.com/foonathan/standardese"
GIT_TAG
    develop
CMAKE_EXTRA_ARGS
    -DLIBCLANG_SYSTEM_INCLUDE_DIR="\"${LIBCLANG_SYSTEM_INCLUDE_DIR}\""
)

set(STANDARDESE_TOOL "${STANDARDESE_BINARY_DIR}/tool/standardese" CACHE INTERNAL "Path to Standardese tool")

function(configure_standardese)
    cmake_parse_arguments(RUNTOOL
        ""
        "TARGET;ROOT_DIR"
        ""
        ${ARGN}
    )

    if(NOT RUNTOOL_TARGET OR NOT TARGET ${RUNTOOL_TARGET})
        message(FATAL_ERROR "No correct target given (${RUNTOOL_TARGET})")
    endif()
    if(NOT RUNTOOL_ROOT_DIR)
        message(FATAL_ERROR "No root dir to scan for headers given")
    endif()

    file(GLOB_RECURSE headers
        ${RUNTOOL_ROOT_DIR}/*.h
        ${RUNTOOL_ROOT_DIR}/*.hpp
    )

    get_target_include_directories(${RUNTOOL_TARGET} include_dirs)
    libclang_system_include_dir(LIBCLANG_SYSTEM_INCLUDE_DIR)
    clangxx_stdlib_includes(libstdc++ STDLIB_INCLUDES)
    list(APPEND include_dirs ${LIBCLANG_SYSTEM_INCLUDE_DIR} ${STDLIB_INCLUDES})

    if(SIPLASPLAS_VERBOSE_CONFIG)
        message(STATUS "Setting up Standardese tool on ${RUNTOOL_TARGET}:")
        message(STATUS "Path to tool: ${STANDARDESE_TOOL}")

        message(STATUS "Headers:")
        foreach(header ${headers})
            message(STATUS "  ${header}")
        endforeach()

        message(STATUS "Include directories:")
        foreach(include_dir ${include_dirs})
            message(STATUS "  ${include_dir}")
        endforeach()
    endif()

    foreach(include_dir ${include_dirs})
        list(APPEND options -I "${include_dir}")
    endforeach()

    add_custom_target(${RUNTOOL_TARGET}-docs
        COMMAND ${STANDARDESE_TOOL} ${options} ${headers}
        DEPENDS standardese
        COMMENT "Running Standardese on ${RUNTOOL_TARGET} headers..."
    )
endfunction()