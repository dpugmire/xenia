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

  xenia::utils::CommandLineArgParser args(argc, argv, {"--json", "--file", "--field", "--isovals"});

  auto jsonFile = args.GetArg("--json")[0];
  auto bpFile = args.GetArg("--file")[0];
  auto argIsoVals = args.GetArg("--isovals");
  std::vector<vtkm::FloatDefault> isoVals;
  for (const auto& val : argIsoVals)
    isoVals.push_back(std::stof(val));

  fides::io::DataSetReader reader(jsonFile);
  std::unordered_map<std::string, std::string> paths;

  paths["source"] = std::string(bpFile);

  auto metaData = reader.ReadMetaData(paths);
  auto data = reader.ReadDataSet(paths, metaData);
  data.PrintSummary(std::cout);

  //generate the contour
  vtkm::filter::contour::Contour contour;
  contour.SetGenerateNormals(false);

  contour.SetActiveField(args.GetArg("--field")[0]);
  for (int i = 0; i < isoVals.size(); i++)
    contour.SetIsoValue(i, isoVals[i]);

  //vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::Mode::All);
  //vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::Mode::None);
  //contour.SetFieldsToPass(selection);

  auto result = contour.Execute(data);

  if (args.HasArg("--output"))
  {
    fides::io::DataSetWriter writer(args.GetArg("--output")[0]);
    //writer.SetWriteFields({"ALL"});
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
