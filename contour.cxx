#include <mpi.h>
#include <vector>
#include <boost/program_options.hpp>

#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <fides/DataSetReader.h>
#include <fides/DataSetWriter.h>

#include <vtkm/filter/contour/Contour.h>

int main(int argc, char** argv)
{
  MPI_Init(NULL, NULL);

  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("file", po::value<std::string>(), "Input file")
    ("json", po::value<std::string>(), "Fides JSON data model file")    
    ("output", po::value<std::string>(), "Output file")
    ("field", po::value<std::string>(), "field name in input data")
    ("isovals", po::value<std::vector<std::string>>(), "Isosurface values")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  
  if (vm.count("help"))
  {
    std::cout << desc << "\n";
    return 1;
  }

  xenia::utils::DataSetReader reader(vm);
  reader.BeginStep();
  auto data = reader.ReadDataSet();
  reader.EndStep();
  std::string fieldName = vm["field"].as<std::string>();
  auto isoVals = vm["isovals"].as<std::vector<vtkm::FloatDefault>>();

  //generate the contour
  vtkm::filter::contour::Contour contour;
  contour.SetGenerateNormals(false);

  contour.SetActiveField(fieldName);
  for (int i = 0; i < isoVals.size(); i++)
    contour.SetIsoValue(i, isoVals[i]);

  vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::Mode::All);
  contour.SetFieldsToPass(selection);

  auto result = contour.Execute(data);

  xenia::utils::WriteData(result, vm["output"].as<std::string>());

  MPI_Finalize();
  return 0;
}
