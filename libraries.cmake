cmake_minimum_required(VERSION 3.16)
function (includeLibraries)

include(FetchContent)
FetchContent_Declare(
  u8g2
  GIT_REPOSITORY    https://github.com/olikraus/u8g2.git
  GIT_TAG           master
  GIT_SHALLOW       1
)
FetchContent_GetProperties(u8g2)

if(NOT u8g2_POPULATED)
  FetchContent_Populate(u8g2)
  execute_process(WORKING_DIRECTORY ${u8g2_SOURCE_DIR} COMMAND bash -c ${CMAKE_SOURCE_DIR}/patch-u8g2.sh )
  add_subdirectory(${u8g2_SOURCE_DIR} ${u8g2_BINARY_DIR})
endif()

FetchContent_MakeAvailable(u8g2)

endfunction()
