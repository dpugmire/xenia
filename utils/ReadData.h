#pragma once

#include "CommandLineArgParser.h"
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <fides/DataSetReader.h>

namespace xenia
{
namespace utils
{

vtkm::cont::PartitionedDataSet
ReadData(const xenia::utils::CommandLineArgParser& args);

}
} //xenia::utils
