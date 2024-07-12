#include <mpi.h>
#include <vector>

#include "utils/CommandLineArgParser.h"
#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <fides/DataSetReader.h>
#include <fides/DataSetWriter.h>

#include <vtkm/filter/contour/Contour.h>

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  xenia::utils::CommandLineArgParser args(argc, argv, {"--file", "--field", "--isovals", "--output"});

  auto argIsoVals = args.GetArg("--isovals");
  std::vector<vtkm::FloatDefault> isoVals;
  for (const auto& val : argIsoVals)
    isoVals.push_back(std::stof(val));

  auto data = xenia::utils::ReadData(args);

  //generate the contour
  vtkm::filter::contour::Contour contour;
  contour.SetGenerateNormals(false);

  contour.SetActiveField(args.GetArg("--field")[0]);
  for (int i = 0; i < isoVals.size(); i++)
    contour.SetIsoValue(i, isoVals[i]);

  vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::Mode::All);
  contour.SetFieldsToPass(selection);

  auto result = contour.Execute(data);

  xenia::utils::WriteData(result, args.GetArg("--output")[0]);

  MPI_Finalize();
  return 0;
}
