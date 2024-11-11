#include <mpi.h>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

#include "utils/CommandLineArgParser.h"
#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <fides/DataSetReader.h>

static void
RunService(xenia::utils::DataSetWriter& writer, vtkm::cont::PartitionedDataSet& pds, const boost::program_options::variables_map& /*vm*/)
{
  writer.BeginStep();
  writer.WriteDataSet(pds);
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

#if 0 //can we take this out??
//old code.
if (0)
{
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();
  std::cout<<"Opening: "<<inputFname<<std::endl;

  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();
  else
    throw std::runtime_error("Error. SST requires a json file.");

  fides::io::DataSetReader reader(jsonFile);
  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  params["engine_type"] = "SST";
  reader.SetDataSourceParameters("source", params);
  }
  #endif

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


  /*

  int step = 0;
  while (true)
  {
    auto status = reader.PrepareNextStep(paths);
    if (status == fides::StepStatus::NotReady)
    {
      std::cout << "Not ready...." << std::endl;
      continue;
    }
    else if (status == fides::StepStatus::EndOfStream)
    {
      std::cout << "Stream is done" << std::endl;
      break;
    }

    fides::metadata::MetaData selections;
    //selections.Set(fides::keys::BLOCK_SELECTION(), blockSelection);

    auto output = reader.ReadDataSet(paths, selections);
    std::cout << "Step: " << step << " #ds= " << output.GetNumberOfPartitions() << std::endl;
    RunService(writer, output, vm);

    step++;
  }
  */
}

int main(int argc, char** argv)
{
#ifdef ENABLE_MPI
  MPI_Init(NULL, NULL);
#endif

  namespace po = boost::program_options;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("file", po::value<std::string>(), "Input file")
    ("json", po::value<std::string>(), "Fides JSON data model file")
    ("output", po::value<std::string>(), "Output file")
    ("input_engine", po::value<std::string>(), "Adios2 input engine type (BP or SST)")
    ("output_engine", po::value<std::string>(), "Adios2 output engine type (BP or SST)")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << desc << "\n";
    return 1;
  }

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
