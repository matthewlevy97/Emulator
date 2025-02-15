cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# CMake Policies
cmake_policy(SET CMP0079 NEW)

option(CMAKE_POSITION_INDEPENDENT_CODE "Default value for POSITION_INDEPENDENT_CODE of targets" TRUE)

project(Emulator LANGUAGES C CXX)
if (APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif()

option(BUILD_TESTS "Build all tests" OFF)

set(FRONTEND_OPTIONS "Desktop")
set(TARGET_FRONTEND "Desktop" CACHE STRING "The frontend to use")

if(NOT TARGET_FRONTEND IN_LIST FRONTEND_OPTIONS)
  message(FATAL_ERROR "Invalid TARGET_FRONTEND: ${TARGET_FRONTEND}. Available options are: ${FRONTEND_OPTIONS}")
endif()

include(FetchContent)

# Add spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.x
)
FetchContent_MakeAvailable(spdlog)

# Add GoogleTest
if(BUILD_TESTS)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        main
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  include(GoogleTest)
endif()

#set(FETCHCONTENT_FULLY_DISCONNECTED ON CACHE BOOL TRUE FORCE)

set(EMULATOR_NAME "emulator")
add_subdirectory(emulator)
