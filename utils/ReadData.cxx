#include "ReadData.h"
#include "CommandLineArgParser.h"

#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <fides/DataSetReader.h>

namespace xenia
{
namespace utils
{

const std::set<std::string> DataSetReader::ValidEngineTypes({"BP5", "SST"});

DataSetReader::DataSetReader(const boost::program_options::variables_map& vm)
{
  std::cout<<"building dataset reader."<<std::endl;
  this->FileName = vm["file"].as<std::string>();
  if (this->FileName.find(".bp") == std::string::npos)
    throw std::string("Error. Only BP files supported.");

  if (!vm["input_engine"].empty())
    this->EngineType = vm["input_engine"].as<std::string>();
  if (this->ValidEngineTypes.find(this->EngineType) == this->ValidEngineTypes.end())
    throw std::runtime_error("Invalid engine type: " + this->EngineType);

  this->Paths["source"] = this->FileName;
  this->Paths["engine_type"] = this->EngineType;

  if (vm.count("json") == 1)
  {
    this->JSONFile = vm["json"].as<std::string>();
    if (this->EngineType == "BP5")
      this->FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(this->JSONFile));
    else if (this->EngineType == "SST")
      this->FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(this->JSONFile, fides::io::DataSetReader::DataModelInput::JSONFile, true));
  }
  else
  {
    //this->FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(this->FileName, fides::io::DataSetReader::DataModelInput::BPFile));
    this->FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(this->FileName));
  }
}


vtkm::cont::PartitionedDataSet DataSetReader::Read()
{
  auto md = this->MetaData;

  if (this->EngineType == "BP5")
    md.Set(fides::keys::STEP_SELECTION(), fides::metadata::Index(this->Step));

  return this->FidesReader->ReadDataSet(this->Paths, md);
}

}
} //xenia::utils
