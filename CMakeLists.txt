cmake_minimum_required(VERSION 3.12...3.15 FATAL_ERROR)
project(Xenia C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_MPI "Enable MPI support" OFF)

# Find the VTK-m package
find_package(VTKm REQUIRED QUIET)
find_package(Fides REQUIRED QUIET)
find_package(Boost REQUIRED COMPONENTS program_options)

set(LINK_LIBS "")
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  list(APPEND LINK_LIBS ${Boost_LIBRARIES})
endif()

# Append Fides, ADIOS and vtkm::io libraries
list(APPEND LINK_LIBS "fides" "vtkm::io" "adios2::adios2")

if (ENABLE_MPI)
  find_package(MPI REQUIRED)
  if (MPI_FOUND)
      message(STATUS "MPI found: ${MPI_CXX_LIBRARIES}")
      include_directories(${MPI_INCLUDE_PATH})
      add_definitions(-DENABLE_MPI)
      include_directories(${MPI_INCLUDE_PATH})
      list(APPEND LINK_LIBS ${MPI_LIBRARIES})  # Use list(APPEND) to append to the list
  else()
      message(FATAL_ERROR "MPI requested but not found")
  endif()
endif()

set(UTIL_HEADERS
  utils/CommandLineArgParser.h
  utils/ReadData.h
  utils/Debug.h
  utils/WriteData.h)
set(UTIL_SRC
  utils/ReadData.cxx
  utils/Debug.cxx
  utils/WriteData.cxx)

set(UTIL_FILES ${UTIL_HEADERS} ${UTIL_SRC})
add_library(xenia_utils SHARED ${UTIL_SRC} ${UTIL_SRC})
target_link_libraries(xenia_utils PRIVATE ${LINK_LIBS} vtkm::filter_entity_extraction)

list(APPEND LINK_LIBS "xenia_utils")

#add_executable(reader reader.cxx)
#target_include_directories(reader PRIVATE utils)
#target_link_libraries(reader PRIVATE fides vtkm::rendering vtkm::filter_contour vtkm::source MPI::MPI_CXX MPI::MPI_C)

#add_executable(bpWriter bpWriter.cxx)
#target_link_libraries(bpWriter PRIVATE vtkm::cont adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

#add_executable(copier copier.cxx)
#target_link_libraries(copier PRIVATE vtkm::io adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

#add_executable(contour contour.cxx)
#target_link_libraries(contour PRIVATE ${Boost_LIBRARIES} xenia_utils fides vtkm::io vtkm::filter_contour adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(converter converter.cxx)
#target_link_libraries(converter PRIVATE ${LINK_LIBS} vtkm::filter_entity_extraction)
target_link_libraries(converter PRIVATE ${LINK_LIBS})

add_executable(service service.cxx)
#target_link_libraries(converter PRIVATE ${LINK_LIBS} vtkm::filter_entity_extraction)
target_link_libraries(service PRIVATE ${LINK_LIBS})

#add_executable(gradient gradient.cxx)
#target_link_libraries(gradient PRIVATE ${LINK_LIBS} vtkm::filter_vector_analysis)

#target_link_libraries(converter PRIVATE xenia_utils ${Boost_LIBRARIES} fides vtkm::io vtkm::filter_contour adios2::adios2)

#add_executable(ext_faces ext_faces.cxx)
#target_link_libraries(ext_faces PRIVATE xenia_utils ${Boost_LIBRARIES} fides vtkm::io vtkm::filter_contour vtkm::filter_entity_extraction  vtkm::filter_clean_grid adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

#add_executable(noop noop.cxx)
#target_link_libraries(noop PRIVATE ${Boost_LIBRARIES} xenia_utils fides vtkm::io adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

#add_executable(cropper cropper.cxx)
#target_link_libraries(cropper PRIVATE xenia_utils ${Boost_LIBRARIES} fides vtkm::io adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

add_executable(streamlines streamlines.cxx)
target_link_libraries(streamlines PRIVATE ${LINK_LIBS} vtkm::filter_flow vtkm::filter_geometry_refinement vtkm::filter_field_transform)

#add_executable(render render.cxx)
#target_link_libraries(render PRIVATE xenia_utils ${Boost_LIBRARIES} fides vtkm::io vtkm::rendering vtkm::source vtkm::filter_contour adios2::adios2 MPI::MPI_CXX MPI::MPI_C)

#add_executable(hello hello.cxx)
#target_link_libraries(hello PRIVATE MPI::MPI_CXX)

## examples
add_subdirectory(examples)

