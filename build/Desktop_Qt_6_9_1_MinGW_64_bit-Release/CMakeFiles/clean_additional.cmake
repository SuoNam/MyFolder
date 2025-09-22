# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\appMyTest_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\appMyTest_autogen.dir\\ParseCache.txt"
  "appMyTest_autogen"
  )
endif()
