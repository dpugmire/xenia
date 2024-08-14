#include <mpi.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/program_options.hpp>

#include <vtkm/filter/entity_extraction/ExternalFaces.h>
#include <vtkm/filter/clean_grid/CleanGrid.h>

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
  MPI_Init(NULL, NULL);

  namespace po = boost::program_options;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("file", po::value<std::string>(), "Input file")
    ("json", po::value<std::string>(), "Fides JSON data model file")        
    ("output", po::value<std::string>(), "Output file")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  xenia::utils::DataSetReader reader(vm);
  reader.BeginStep();
  auto data = reader.ReadDataSet();
  reader.EndStep();

  data = ExternalFaces(data);

  xenia::utils::WriteData(data, vm["output"].as<std::string>());

  MPI_Finalize();
  return 0;
}
