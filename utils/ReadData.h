#pragma once

#include "CommandLineArgParser.h"
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <fides/DataSetReader.h>
#include <boost/program_options.hpp>

namespace xenia
{
namespace utils
{

vtkm::cont::PartitionedDataSet
ReadData(const xenia::utils::CommandLineArgParser& args);

vtkm::cont::PartitionedDataSet
ReadData(const boost::program_options::variables_map& vm);

fides::io::DataSetReader*
CreateDataSetReader(const boost::program_options::variables_map& vm);

class DataSetReader
{
  public:
  DataSetReader(const boost::program_options::variables_map& vm);

  void Init()
  { 
    this->MetaData = this->FidesReader->ReadMetaData(this->Paths);
  }


  fides::io::DataSetReader *FidesReader = nullptr;
  std::unordered_map<std::string, std::string> Paths;
  std::string JSONFile = "";
  std::string FileName = "";
  fides::metadata::MetaData MetaData;
};

}
} //xenia::utils
