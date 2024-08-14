#include "ReadData.h"
#include "CommandLineArgParser.h"

#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <fides/DataSetReader.h>

namespace xenia
{
namespace utils
{
#if 0
vtkm::cont::PartitionedDataSet
ReadData(const xenia::utils::CommandLineArgParser& args)
{
  vtkm::cont::PartitionedDataSet pds;
  std::unordered_map<std::string, std::string> paths;

  auto inputFname = args.GetArg("--file")[0];

  if (inputFname.find(".vtk") != std::string::npos)
  {
    vtkm::io::VTKDataSetReader reader(inputFname);
    auto ds = reader.ReadDataSet();
    pds.AppendPartition(ds);
  }
  else if (inputFname.find(".bp") != std::string::npos)
  {
    auto bpFile = args.GetArg("--file")[0];
    paths["source"] = std::string(inputFname);
    if (args.HasArg("--json"))
    {
      auto jsonFile = args.GetArg("--json")[0];
      std::cout<<"Reading: w/ json "<<inputFname<<" "<<jsonFile<<std::endl;

      fides::io::DataSetReader reader(jsonFile);
      auto metaData = reader.ReadMetaData(paths);
      pds = reader.ReadDataSet(paths, metaData);
      std::cout<<"Read done"<<std::endl;
    }
    else
    {
      std::cout<<"Reading: w/ attrs "<<inputFname<<std::endl;
      fides::io::DataSetReader reader(inputFname, fides::io::DataSetReader::DataModelInput::BPFile);
      auto metaData = reader.ReadMetaData(paths);
      pds = reader.ReadDataSet(paths, metaData);
      std::cout<<"Read done"<<std::endl;
    }
  }
  else
  {
    std::cerr<<" Error. Unknown file type "<<inputFname<<std::endl;
  }

  return pds;
}

vtkm::cont::PartitionedDataSet
ReadData(const boost::program_options::variables_map& vm)
{
  vtkm::cont::PartitionedDataSet pds;
  std::unordered_map<std::string, std::string> paths;

  auto inputFname = vm["file"].as<std::string>();

  if (inputFname.find(".vtk") != std::string::npos)
  {
    vtkm::io::VTKDataSetReader reader(inputFname);
    auto ds = reader.ReadDataSet();
    pds.AppendPartition(ds);
  }
  else if (inputFname.find(".bp") != std::string::npos)
  {
    paths["source"] = std::string(inputFname);
    if (vm.count("json") == 1)
    {
      auto jsonFile = vm["json"].as<std::string>();
      std::cout<<"Reading: w/ json "<<inputFname<<" "<<jsonFile<<std::endl;

      fides::io::DataSetReader reader(jsonFile);
      auto metaData = reader.ReadMetaData(paths);
      pds = reader.ReadDataSet(paths, metaData);
      std::cout<<"Read done"<<std::endl;
    }
    else
    {
      std::cout<<"Reading: w/ attrs "<<inputFname<<std::endl;
      fides::io::DataSetReader reader(inputFname, fides::io::DataSetReader::DataModelInput::BPFile);
      auto metaData = reader.ReadMetaData(paths);
      pds = reader.ReadDataSet(paths, metaData);
      std::cout<<"Read done"<<std::endl;
    }
  }
  else
  {
    std::cerr<<" Error. Unknown file type "<<inputFname<<std::endl;
  }

  return pds;
}


fides::io::DataSetReader*
CreateDataSetReader(const boost::program_options::variables_map& vm)
{
  std::unordered_map<std::string, std::string> paths;
  auto inputFname = vm["file"].as<std::string>();
  if(inputFname.find(".bp") == std::string::npos)
  {
    std::cerr<<" Error: Only BP files supported.";
    return nullptr;
  }
  
  paths["source"] = std::string(inputFname);
  fides::io::DataSetReader* reader = nullptr;

  if (vm.count("json") == 1)
  {
    auto jsonFile = vm["json"].as<std::string>();
    std::cout<<"Reading: w/ json "<<inputFname<<" "<<jsonFile<<std::endl;

    reader = new fides::io::DataSetReader(jsonFile);
  }
  else
  {
    std::cout<<"Reading: w/ attrs "<<inputFname<<std::endl;
    paths["source"] = inputFname;

    reader = new fides::io::DataSetReader(inputFname, fides::io::DataSetReader::DataModelInput::BPFile);
  }

  return reader;
}
#endif

DataSetReader::DataSetReader(const boost::program_options::variables_map& vm)
{
  std::cout<<"building dataset reader."<<std::endl;
  this->FileName = vm["file"].as<std::string>();
  if (this->FileName.find(".bp") == std::string::npos)
    throw std::string("Error. Only BP files supported.");

  this->Paths["source"] = this->FileName;
  if (vm.count("json") == 1)
  {
    this->JSONFile = vm["json"].as<std::string>();
    this->FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(this->JSONFile));
  }
  else
  {
    this->FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(this->FileName, fides::io::DataSetReader::DataModelInput::BPFile));
  }
}

}
} //xenia::utils
