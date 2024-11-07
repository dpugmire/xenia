#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#

find_package(ADIOS2 REQUIRED QUIET)

add_executable(gray-scott
 main.cpp
 gray-scott.cpp
 settings.cpp
 writer.cpp
 restart.cpp
 settings.cpp
)

target_link_libraries(gray-scott PRIVATE fides adios2::adios2 MPI::MPI_C MPI::MPI_CXX)
