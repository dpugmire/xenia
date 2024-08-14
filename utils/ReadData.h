#pragma once

#include <memory>
#include "CommandLineArgParser.h"
#include <vtkm/io/VTKDataSetReader.h>

#include <fides/DataSetReader.h>
#include <boost/program_options.hpp>

namespace xenia
{
namespace utils
{
class DataSetReader
{
  public:
  DataSetReader(const boost::program_options::variables_map& vm);

  void Init()
  { 
    this->MetaData = this->FidesReader->ReadMetaData(this->Paths);
    this->InitCalled = true;
  }

  fides::StepStatus BeginStep()
  {
    if (!this->InitCalled)
    {
      throw std::runtime_error("Error: Init must be called before BeginStep().");
    }

    auto status = this->FidesReader->PrepareNextStep(this->Paths);
    return status;
  }
  void EndStep()
  {
    this->Step++;
  }

  vtkm::cont::PartitionedDataSet
  ReadDataSet()
  {
    return this->FidesReader->ReadDataSet(this->Paths, this->MetaData);
  }


  vtkm::Id Step = 0;
  std::unique_ptr<fides::io::DataSetReader> FidesReader;
  std::unordered_map<std::string, std::string> Paths;
  fides::metadata::MetaData MetaData;  
  std::string JSONFile = "";
  std::string FileName = "";
  bool InitCalled = false;
};

}
} //xenia::utils
