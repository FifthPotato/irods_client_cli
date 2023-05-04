cmake_minimum_required(VERSION 3.7.0 FATAL_ERROR)

find_package(IRODS REQUIRED)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(IRODS_CLI_REVISION "0")
set(IRODS_CLI_VERSION "${IRODS_VERSION}.${IRODS_CLI_REVISION}")
set(irods_static_subcommands ls put get repl touch cp rm pwd cd)
set(irods_dynamic_subcommands)

set(CMAKE_C_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang)
set(CMAKE_CXX_COMPILER ${IRODS_EXTERNALS_FULLPATH_CLANG}/bin/clang++)

project(irods_cli
  VERSION "${IRODS_CLI_VERSION}"
  LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(IRODS_CLI_COMMANDS_INSTALL_DIR ${IRODS_PLUGINS_DIRECTORY}/cli)

include(${IRODS_TARGETS_PATH})

include(GNUInstallDirs)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build {Debug, Release}." FORCE)
    message(STATUS "Setting unspecified CMAKE_BUILD_TYPE to '${CMAKE_BUILD_TYPE}'")
endif()

find_package(nlohmann_json "3.6.1" REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto SSL)

if (NOT IRODS_EXTERNALS_PACKAGE_ROOT)
    set(IRODS_EXTERNALS_PACKAGE_ROOT "/opt/irods-externals" CACHE STRING "Choose the location of iRODS external packages." FORCE)
    message(STATUS "Setting unspecified IRODS_EXTERNALS_PACKAGE_ROOT to '${IRODS_EXTERNALS_PACKAGE_ROOT}'")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(CMAKE_INSTALL_RPATH ${IRODS_EXTERNALS_FULLPATH_CLANG_RUNTIME}/lib)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++ -Wl,--export-dynamic -pthread")

# Make ALL symbols in shared libraries hidden by default.
# TODO Verify this behavior.
set_property(GLOBAL PROPERTY CMAKE_CXX_VISIBILITY_PRESET hidden)

set(APP irods)

add_executable(${APP} ${CMAKE_SOURCE_DIR}/src/main.cpp)

set_target_properties(${APP} PROPERTIES CXX_STANDARD ${IRODS_CXX_STANDARD})

target_compile_options(${APP} PRIVATE -Wno-write-strings -nostdinc++)

target_compile_definitions(${APP} PRIVATE IRODS_CLI_VERSION="${IRODS_CLI_VERSION}" ${IRODS_COMPILE_DEFINITIONS})


target_include_directories(${APP} PRIVATE ${CMAKE_SOURCE_DIR}/include
                                          ${IRODS_INCLUDE_DIRS}
                                          ${IRODS_EXTERNALS_FULLPATH_BOOST}/include
                                          ${IRODS_EXTERNALS_FULLPATH_CLANG}/include/c++/v1
                                          ${IRODS_EXTERNALS_FULLPATH_FMT}/include)


list(TRANSFORM irods_static_subcommands PREPEND irods_cli_ OUTPUT_VARIABLE static_liblist)
target_link_libraries(${APP} PRIVATE irods_common
                                     irods_plugin_dependencies
                                     ${static_liblist}
                                     ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_filesystem.so
                                     ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_system.so
                                     ${IRODS_EXTERNALS_FULLPATH_BOOST}/lib/libboost_program_options.so
                                     ${IRODS_EXTERNALS_FULLPATH_FMT}/lib/libfmt.so
                                     dl)

list(JOIN irods_static_subcommands ")(" cli_boost_seq)
set(cli_boost_seq "(${cli_boost_seq})")
target_compile_definitions(${APP} PRIVATE CLI_SEQ=${cli_boost_seq})

# CLI Commands
set(do_static TRUE)
foreach(cmd ${irods_static_subcommands})
    add_subdirectory(commands/${cmd})
    target_compile_definitions(irods_cli_${cmd} PRIVATE DO_STATIC)
endforeach()
set(do_static)
foreach(cmd ${irods_dynamic_subcommands})
    add_subdirectory(commands/${cmd})
endforeach()

#add_subdirectory(commands/cp)
#add_subdirectory(commands/get)
#add_subdirectory(commands/repl)
#add_subdirectory(commands/rm)
#add_subdirectory(commands/touch)

# Installation
install(TARGETS ${APP}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)

