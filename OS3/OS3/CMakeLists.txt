# CMakeList.txt : CMake project for OS3, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (OS3 "OS3.cpp" "OS3.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET OS3 PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
