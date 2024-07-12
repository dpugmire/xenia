#include <mpi.h>
#include <string>
#include <unordered_map>
#include <vector>

#include <vtkm/filter/entity_extraction/ExternalFaces.h>
#include <vtkm/filter/clean_grid/CleanGrid.h>

#include "utils/CommandLineArgParser.h"
#include "utils/ReadData.h"
#include "utils/WriteData.h"

static vtkm::cont::PartitionedDataSet
ExternalFaces(const vtkm::cont::PartitionedDataSet& inData)
{
  vtkm::filter::entity_extraction::ExternalFaces extFilter;
  vtkm::filter::clean_grid::CleanGrid cleanFilter;

  vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::Mode::All);
  extFilter.SetFieldsToPass(selection);
  auto outData = extFilter.Execute(inData);
  return cleanFilter.Execute(outData);
}

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  xenia::utils::CommandLineArgParser args(argc, argv, {"--file", "--output"});

  auto data = xenia::utils::ReadData(args);
  data = ExternalFaces(data);

  xenia::utils::WriteData(data, args.GetArg("--output")[0]);

  MPI_Finalize();
  return 0;
}
