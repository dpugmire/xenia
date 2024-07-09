cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(VTKmDemo C CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
find_package(Fides REQUIRED QUIET)
find_package(adios2 REQUIRED QUIET)

add_executable(reader reader.cxx)
target_include_directories(reader PRIVATE utils)
target_link_libraries(reader PRIVATE fides vtkm::rendering vtkm::filter_contour vtkm::source MPI::MPI_CXX MPI::MPI_C)

add_executable(bpWriter bpWriter.cxx)
target_link_libraries(bpWriter PRIVATE vtkm::cont adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(copier copier.cxx)
target_link_libraries(copier PRIVATE vtkm::io adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(contour contour.cxx)
target_link_libraries(contour PRIVATE fides vtkm::io vtkm::filter_contour adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(converter converter.cxx)
target_link_libraries(converter PRIVATE fides vtkm::io vtkm::filter_contour adios2::adios2 MPI::MPI_CXX MPI::MPI_C)
