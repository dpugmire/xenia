#pragma once

#include <memory>
#include "CommandLineArgParser.h"
#include <vtkm/io/VTKDataSetReader.h>

#include <fides/DataSetReader.h>
#include <boost/program_options.hpp>

#ifdef ENABLE_MPI
#include <mpi.h>
#endif


namespace xenia
{
namespace utils
{
class DataSetReader
{
  public:
  DataSetReader(const boost::program_options::variables_map& vm);

  void Init();
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


private:
  void SetBlocksMetaData(fides::metadata::MetaData& md) const;
  void InitBlockSelection();

  vtkm::Id Step = 0;
  std::unique_ptr<fides::io::DataSetReader> FidesReader;
  std::unordered_map<std::string, std::string> Paths;
  fides::metadata::MetaData MetaData;
  std::vector<std::size_t> BlockSelection;
  std::string JSONFile = "";
  std::string FileName = "";
  std::string EngineType = "BP5";
  bool InitCalled = false;
  vtkm::Id NumSteps = 0;

  int Rank = 0;
  int NumRanks = 1;

  static const std::set<std::string> ValidEngineTypes;
};

}
} //xenia::utils
