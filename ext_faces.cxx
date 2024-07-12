#include <mpi.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <fides/DataSetReader.h>
#include <fides/DataSetWriter.h>
#include <vtkm/filter/entity_extraction/ExternalFaces.h>
#include <vtkm/filter/clean_grid/CleanGrid.h>

#include "utils/CommandLineArgParser.h"

static void
DumpPartitions(const vtkm::cont::PartitionedDataSet& pds, const std::string& outName)
{
  fides::io::DataSetWriter writer(outName);
  //writer.SetWriteFields({"ALL"});
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
    std::cout<<"Reading: w/ json "<<bpFile<<std::endl;
    auto jsonFile = args.GetArg("--json")[0];
    fides::io::DataSetReader reader(jsonFile);

    auto metaData = reader.ReadMetaData(paths);
    pds = reader.ReadDataSet(paths, metaData);
  }
  else
  {
    std::cout<<"Reading: w/ attrs "<<bpFile<<std::endl;
    fides::metadata::Vector<std::size_t> blockSelection;
    blockSelection.Data.push_back(0);
    fides::io::DataSetReader reader(bpFile, fides::io::DataSetReader::DataModelInput::BPFile);
    //auto status = reader.PrepareNextStep(paths);
    auto metaData = reader.ReadMetaData(paths);
    //metaData.Set(fides::keys::BLOCK_SELECTION(), blockSelection);

    pds = reader.ReadDataSet(paths, metaData);
  }

  return pds;
}

static vtkm::cont::PartitionedDataSet
ExternalFaces(const vtkm::cont::PartitionedDataSet& inData)
{
  vtkm::filter::entity_extraction::ExternalFaces extFilter;
  vtkm::filter::clean_grid::CleanGrid cleanFilter;

  vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::Mode::None);
  extFilter.SetFieldsToPass(selection);
  auto outData = extFilter.Execute(inData);
  return cleanFilter.Execute(outData);
}

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  xenia::utils::CommandLineArgParser args(argc, argv, {"--file", "--output"});

  auto data = ReadPartitions(args);
  data = ExternalFaces(data);
  data.PrintSummary(std::cout);

  DumpPartitions(data, args.GetArg("--output")[0]);

  MPI_Finalize();
  return 0;
}
