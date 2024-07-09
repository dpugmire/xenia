#include <mpi.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <fides/DataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include "utils/CommandLineArgParser.h"

static void
DumpPartitions(const vtkm::cont::PartitionedDataSet& pds, const std::string& outName)
{
  for (vtkm::Id i = 0; i < pds.GetNumberOfPartitions(); i++)
  {
    std::string fname = outName + std::to_string(i) + ".vtk";
    vtkm::io::VTKDataSetWriter writer(fname);
    writer.SetFileTypeToBinary();
    writer.WriteDataSet(pds.GetPartition(i));
  }
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
    fides::io::DataSetReader reader(jsonFile);

    auto metaData = reader.ReadMetaData(paths);
    pds = reader.ReadDataSet(paths, metaData);
  }
  else
  {
    std::cout<<"Reading: "<<bpFile<<std::endl;
    fides::metadata::Vector<std::size_t> blockSelection;
    blockSelection.Data.push_back(0);
    fides::io::DataSetReader reader(bpFile, fides::io::DataSetReader::DataModelInput::BPFile);
    auto metaData = reader.ReadMetaData(paths);
    metaData.Set(fides::keys::BLOCK_SELECTION(), blockSelection);

    pds = reader.ReadDataSet(paths, metaData);
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
