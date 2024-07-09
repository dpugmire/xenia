#include <mpi.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <fides/DataSetReader.h>
#include <fides/DataSetWriter.h>

#include "utils/CommandLineArgParser.h"
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/io/VTKDataSetWriter.h>


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

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  xenia::utils::CommandLineArgParser args(argc, argv, {"--json", "--file", "--field"});

  auto jsonFile = args.GetArg("--json")[0];
  auto bpFile = args.GetArg("--file")[0];

  fides::io::DataSetReader reader(jsonFile);
  std::unordered_map<std::string, std::string> paths;

  paths["source"] = std::string(bpFile);

  auto metaData = reader.ReadMetaData(paths);
  auto data = reader.ReadDataSet(paths, metaData);

  //generate the contour
  vtkm::filter::contour::Contour contour;

  contour.SetActiveField(args.GetArg("--field")[0]);
  contour.SetIsoValue(0, 5);
  contour.SetGenerateNormals(true);

  auto result = contour.Execute(data);

  result.PrintSummary(std::cout);

  if (args.HasArg("--output"))
  {
    fides::io::DataSetWriter writer(args.GetArg("--output")[0]);
    writer.Write(result, "BPFile");
  }
  else if (args.HasArg("--output_vtk"))
  {
    //DumpPartitions(result, args.GetArg("--output_vtk")[0]);
    DumpPartitions(result, args.GetArg("--output_vtk")[0]);
  }

  MPI_Finalize();
  return 0;
}
