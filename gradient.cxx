#include <mpi.h>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

#include "utils/Debug.h"
#include "utils/CommandLineArgParser.h"
#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <fides/DataSetReader.h>

#include <vtkm/filter/vector_analysis/Gradient.h>

static void
RunService(xenia::utils::DataSetWriter& writer, vtkm::cont::PartitionedDataSet& pds, const boost::program_options::variables_map& vm)
{
  std::string inputFieldName = vm["field"].as<std::string>();
  std::string outputFieldName = vm["output_field"].as<std::string>();

  vtkm::filter::vector_analysis::Gradient filter;
  filter.SetActiveField(inputFieldName);
  filter.SetOutputFieldName(outputFieldName);

  bool doCell = true;

  if (vm.count("point") == 1)
    filter.SetComputePointGradient(true);
  else
    filter.SetComputePointGradient(false);

auto result = filter.Execute(pds);

  writer.BeginStep();
  writer.WriteDataSet(result);
  writer.EndStep();
}

static void
RunBP(const boost::program_options::variables_map& vm)
{
  std::cout<<"RunBP"<<std::endl;
  xenia::utils::DataSetReader reader(vm);
  xenia::utils::DataSetWriter writer(vm);
  reader.Init();

  vtkm::Id numSteps = reader.GetNumSteps();

  for (vtkm::Id step = 0; step < numSteps; step++)
  {
    reader.BeginStep();
    auto output = reader.Read();
    std::cout<<reader.GetRank()<<": has "<<output.GetNumberOfPartitions()<<std::endl;

    RunService(writer, output, vm);

    reader.EndStep();
  }

  writer.Close();
}

static void
RunSST(const boost::program_options::variables_map& vm)
{
  std::cout<<"RunSST"<<std::endl;
  xenia::utils::DataSetWriter writer(vm);
  xenia::utils::DataSetReader reader(vm);

  reader.Init();
  vtkm::Id step = 0;
  while (true)
  {
    auto status = reader.BeginStep();

    if (status == fides::StepStatus::NotReady)
      continue;
    else if (status == fides::StepStatus::EndOfStream)
      break;
    else if (status == fides::StepStatus::OK)
    {
      std::cout<<"SST read step= "<<step<<std::endl;
      auto output = reader.ReadDataSet();
      std::cout<<"    np= "<<output.GetNumberOfPartitions()<<std::endl;
      step++;
    }
    else
      throw std::runtime_error("Unexpected status.");
  }
}

int main(int argc, char** argv)
{
#ifdef ENABLE_MPI
  MPI_Init(NULL, NULL);
#endif

  //InitDebug();

  namespace po = boost::program_options;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("file", po::value<std::string>(), "Input file")
    ("json", po::value<std::string>(), "Fides JSON data model file")
    ("field", po::value<std::string>(), "field name in input data")
    ("output_field", po::value<std::string>(), "field name for gradient field")
    ("point", "compute gradient at vertex points")
    ("cell", "compute gradient at cell")
    ("output", po::value<std::string>(), "Output file")
    ("input_engine", po::value<std::string>(), "Adios2 input engine type (BP or SST")
    ("output_engine", po::value<std::string>(), "Adios2 output engine type (BP or SST")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  std::string engineType = "BP5";
  if (!vm["input_engine"].empty())
    engineType = vm["input_engine"].as<std::string>();

  if (engineType == "SST")
    RunSST(vm);
  else
    RunBP(vm);

#ifdef ENABLE_MPI
  MPI_Finalize();
#endif
  return 0;
}
