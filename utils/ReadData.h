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

    //For BPfile method...
    if (this->EngineType == "BP5")
    {
      if (this->MetaData.Has(fides::keys::NUMBER_OF_STEPS()))
        this->NumSteps = this->MetaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems;
      else
        this->NumSteps = 1;
      std::cout<<"***** NSTEPS= "<<this->NumSteps<<std::endl;
    }
    else if (this->EngineType == "SST")
    {
      std::cout<<"SST init."<<std::endl;
    }

  }

  vtkm::Id GetNumSteps() const { return this->NumSteps; }

  vtkm::cont::PartitionedDataSet Read();

  fides::StepStatus BeginStep()
  {
    if (!this->InitCalled)
      throw std::runtime_error("Error: Init must be called before BeginStep().");

    fides::StepStatus status = fides::StepStatus::OK;
    if (this->EngineType == "SST")
      status = this->FidesReader->PrepareNextStep(this->Paths);

    return status;
  }
  void EndStep()
  {
    this->Step++;
  }

  vtkm::cont::PartitionedDataSet
  ReadDataSet(vtkm::Id step)
  {
    auto selections = this->MetaData;
    selections.Set(fides::keys::STEP_SELECTION(), fides::metadata::Index(step));
    return this->FidesReader->ReadDataSet(this->Paths, selections);
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
  std::string EngineType = "BP5";
  bool InitCalled = false;
  vtkm::Id NumSteps = 0;

  static const std::set<std::string> ValidEngineTypes;
};

}
} //xenia::utils
