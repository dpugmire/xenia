#include "ReadData.h"
#include "CommandLineArgParser.h"

#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <fides/DataSetReader.h>

namespace xenia
{
namespace utils
{

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

}
} //xenia::utils
