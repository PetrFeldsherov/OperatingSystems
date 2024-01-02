# CMakeList.txt : CMake project for Colloqium_operations_from_file, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (Colloqium_operations_from_file "Colloqium_operations_from_file.cpp" "Colloqium_operations_from_file.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Colloqium_operations_from_file PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
