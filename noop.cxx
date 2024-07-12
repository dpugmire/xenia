#include <mpi.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <fides/DataSetReader.h>
#include <fides/DataSetWriter.h>

#include "utils/CommandLineArgParser.h"

static void
DumpPartitions(const vtkm::cont::PartitionedDataSet& pds, const std::string& outName)
{
  std::cout<<"Writing: "<<outName<<std::endl;
  fides::io::DataSetWriter writer(outName);
  writer.Write(pds, "BPFile");
}

static vtkm::cont::PartitionedDataSet
ReadPartitions(const xenia::utils::CommandLineArgParser& args)
{
  vtkm::cont::PartitionedDataSet pds;
  std::unordered_map<std::string, std::string> paths;

  auto bpFile = args.GetArg("--file")[0];
  paths["source"] = std::string(bpFile);
  if (args.HasArg("--json"))
  {
    auto jsonFile = args.GetArg("--json")[0];
    std::cout<<"Reading: w/ json "<<bpFile<<" "<<jsonFile<<std::endl;

    fides::io::DataSetReader reader(jsonFile);
    auto status = reader.PrepareNextStep(paths);
    auto metaData = reader.ReadMetaData(paths);
    pds = reader.ReadDataSet(paths, metaData);
    std::cout<<"Read done"<<std::endl;
  }
  else
  {
    std::cout<<"Reading: w/ attrs "<<bpFile<<std::endl;
    fides::io::DataSetReader reader(bpFile, fides::io::DataSetReader::DataModelInput::BPFile);
    //auto status = reader.PrepareNextStep(paths);
    auto metaData = reader.ReadMetaData(paths);

    pds = reader.ReadDataSet(paths, metaData);
    std::cout<<"Read done"<<std::endl;
  }

  return pds;
}

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  xenia::utils::CommandLineArgParser args(argc, argv, {"--file", "--output"});

  auto data = ReadPartitions(args);

  DumpPartitions(data, args.GetArg("--output")[0]);

  MPI_Finalize();
  return 0;
}
