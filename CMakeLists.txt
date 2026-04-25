cmake_minimum_required(VERSION 3.16)
project(
  cpp_yyjson
  VERSION 0.6.0
  LANGUAGES CXX)

# check if the project is top-level
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CPPYYJSON_IS_TOPLEVEL_PROJECT ON)
else()
  set(CPPYYJSON_IS_TOPLEVEL_PROJECT OFF)
endif()

# options
option(CPPYYJSON_BUILD_TEST "Build ${PROJECT_NAME} tests"
       ${CPPYYJSON_IS_TOPLEVEL_PROJECT})
option(CPPYYJSON_BUILD_BENCH "Build ${PROJECT_NAME} tests and benchmarks" OFF)
option(CPPYYJSON_USE_STD_FORMAT "Use std::format instead of {fmt}" ON)

# library
include(GNUInstallDirs)
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_include_directories(
  ${PROJECT_NAME}
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

# for MSVC
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  target_compile_options(${PROJECT_NAME} INTERFACE /Zc:preprocessor)
endif()

# dependencies
include(FetchContent)

function(find_and_link package_name qualified_name)
  cmake_parse_arguments(FIND_AND_LINK "" "VERSION" "" ${ARGN})
  if(NOT TARGET ${package_name} OR CPPYYJSON_IS_TOPLEVEL_PROJECT)
    find_package(${package_name} ${FIND_AND_LINK_VERSION} CONFIG QUIET)
    if(NOT ${package_name}_FOUND)
      return()
    endif()
    target_link_libraries(${PROJECT_NAME} INTERFACE ${qualified_name})
  else()
    target_link_libraries(${PROJECT_NAME} INTERFACE ${package_name})
  endif()
endfunction()

find_and_link(yyjson yyjson::yyjson)

if(NOT yyjson_FOUND)
  FetchContent_Declare(
    yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson
    GIT_TAG 0.12.0
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(yyjson)
  target_link_libraries(${PROJECT_NAME} INTERFACE yyjson)
endif()
# Validate compiler support for std::format
if(CPPYYJSON_USE_STD_FORMAT)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13")
      message(FATAL_ERROR
        "std::format requires GCC >= 13 (found ${CMAKE_CXX_COMPILER_VERSION}). "
        "Set -DCPPYYJSON_USE_STD_FORMAT=OFF to use {fmt} instead."
      )
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18")
      message(FATAL_ERROR
        "std::format requires Clang >= 18 (found ${CMAKE_CXX_COMPILER_VERSION}). "
        "Set -DCPPYYJSON_USE_STD_FORMAT=OFF to use {fmt} instead."
      )
    endif()
  endif()
endif()

if(NOT CPPYYJSON_USE_STD_FORMAT)
  find_and_link(fmt fmt::fmt-header-only VERSION 10.0.0)
endif()
find_and_link(nameof nameof::nameof VERSION 0.10.0)

# boost::pfr: find_package with CPPYYJSON_PFR_TARGET fallback
if(NOT CPPYYJSON_PFR_TARGET)
  if(NOT TARGET Boost::pfr OR CPPYYJSON_IS_TOPLEVEL_PROJECT)
    find_package(boost_pfr CONFIG QUIET)
    if(boost_pfr_FOUND)
      set(CPPYYJSON_PFR_TARGET "Boost::pfr" CACHE STRING "CMake target for boost::pfr")
    endif()
  else()
    set(CPPYYJSON_PFR_TARGET "Boost::pfr" CACHE STRING "CMake target for boost::pfr")
  endif()
endif()

if(CPPYYJSON_PFR_TARGET)
  if(NOT TARGET ${CPPYYJSON_PFR_TARGET})
    message(FATAL_ERROR
      "CPPYYJSON_PFR_TARGET is set to '${CPPYYJSON_PFR_TARGET}' "
      "but this target does not exist. Please verify the target name."
    )
  endif()
  target_link_libraries(${PROJECT_NAME} INTERFACE ${CPPYYJSON_PFR_TARGET})
else()
  message(FATAL_ERROR
    "boost::pfr is required but was not found.\n"
    "Options:\n"
    "  1. Install boost::pfr (e.g., via vcpkg: vcpkg install boost-pfr)\n"
    "  2. Set -DCPPYYJSON_PFR_TARGET=<target> to specify an existing CMake target"
  )
endif()

# Propagate format backend choice as compile definition
if(CPPYYJSON_USE_STD_FORMAT)
  target_compile_definitions(${PROJECT_NAME} INTERFACE CPPYYJSON_USE_STD_FORMAT)
endif()

# install
if(CPPYYJSON_IS_TOPLEVEL_PROJECT)
  install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}_Targets)

  install(
    EXPORT ${PROJECT_NAME}_Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    ${PROJECT_NAME}ConfigVersion.cmake COMPATIBILITY SameMajorVersion
                                                     ARCH_INDEPENDENT)
  configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

  # install config files
  install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

  # install header files
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

# test and benchmark
if(CPPYYJSON_BUILD_TEST OR CPPYYJSON_BUILD_BENCH)
  enable_testing()
  add_subdirectory(test)
endif()
