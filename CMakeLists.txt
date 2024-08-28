cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
#cmake_minimum_required(VERSION 3.25)
project(VTKmDemo C CXX)

#Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
find_package(Fides REQUIRED QUIET)
#find_package(adios2 REQUIRED QUIET)
find_package(Boost REQUIRED COMPONENTS program_options)
#find_package(Boost COMPONENTS program_options)
#find_package(Boost 1.85.0 REQUIRED QUIET)
#find_package(Boost CONFIG)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(UTIL_HEADERS
  utils/CommandLineArgParser.h
  utils/ReadData.h
  utils/WriteData.h)
set(UTIL_SRC
  utils/ReadData.cxx
  utils/WriteData.cxx)

set(UTIL_FILES ${UTIL_HEADERS} ${UTIL_SRC})
add_library(xenia_utils SHARED ${UTIL_SRC} ${UTIL_SRC})
target_link_libraries(xenia_utils PRIVATE ${Boost_LIBRARIES} fides vtkm::io vtkm::cont)

#add_executable(reader reader.cxx)
#target_include_directories(reader PRIVATE utils)
#target_link_libraries(reader PRIVATE fides vtkm::rendering vtkm::filter_contour vtkm::source MPI::MPI_CXX MPI::MPI_C)

add_executable(bpWriter bpWriter.cxx)
target_link_libraries(bpWriter PRIVATE vtkm::cont adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(copier copier.cxx)
target_link_libraries(copier PRIVATE vtkm::io adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(contour contour.cxx)
target_link_libraries(contour PRIVATE ${Boost_LIBRARIES} xenia_utils fides vtkm::io vtkm::filter_contour adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(converter converter.cxx)
target_link_libraries(converter PRIVATE xenia_utils ${Boost_LIBRARIES} fides vtkm::io vtkm::filter_contour adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(ext_faces ext_faces.cxx)
target_link_libraries(ext_faces PRIVATE xenia_utils ${Boost_LIBRARIES} fides vtkm::io vtkm::filter_contour vtkm::filter_entity_extraction  vtkm::filter_clean_grid adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(noop noop.cxx)
target_link_libraries(noop PRIVATE ${Boost_LIBRARIES} xenia_utils fides vtkm::io adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(cropper cropper.cxx)
target_link_libraries(cropper PRIVATE xenia_utils ${Boost_LIBRARIES} fides vtkm::io adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(render render.cxx)
target_link_libraries(render PRIVATE xenia_utils ${Boost_LIBRARIES} fides vtkm::io vtkm::rendering adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(hello hello.cxx)
target_link_libraries(hello PRIVATE MPI::MPI_CXX)

## examples
add_subdirectory(examples)

