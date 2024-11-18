#include <mpi.h>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <unistd.h> // for sleep()

#include "utils/Debug.h"
#include "utils/CommandLineArgParser.h"
#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/CellClassification.h>
#include <fides/DataSetReader.h>

static vtkm::cont::PartitionedDataSet
RunService(const vtkm::cont::PartitionedDataSet& input,
	         const boost::program_options::variables_map& /*vm*/)
{
  vtkm::cont::PartitionedDataSet output;

  output = input;
  
  return output;
}

static void
RunService2(xenia::utils::DataSetWriter& writer, const vtkm::cont::PartitionedDataSet& pds, const boost::program_options::variables_map& /*vm*/)
{
  writer.BeginStep();
  writer.WriteDataSet(pds);
  writer.EndStep();
}

static void
RunVTK(const boost::program_options::variables_map& vm)
{
  std::cout << "RunVTK" << std::endl;
  vtkm::io::VTKDataSetReader reader(vm["file"].as<std::string>());
  xenia::utils::DataSetWriter writer(vm);

  auto output = reader.ReadDataSet();

  RunService2(writer, vtkm::cont::PartitionedDataSet{ output }, vm);

  writer.Close();
}

static void
RunBP(const boost::program_options::variables_map& vm)
{
  int rank = 0, numProcs = 1;
#ifdef ENABLE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
#endif
  std::cout<<"RunBP"<<std::endl;
  xenia::utils::DataSetReader reader(vm);
  xenia::utils::DataSetWriter writer(vm);
  reader.Init();

  vtkm::Id numSteps = reader.GetNumSteps();

  for (vtkm::Id step = 0; step < numSteps; step++)
  {
    reader.BeginStep();
    auto output = reader.Read();
    std::cout<<rank<<": has "<<output.GetNumberOfPartitions()<<std::endl;

    RunService2(writer, output, vm);

    reader.EndStep();
  }

  writer.Close();
}

vtkm::cont::PartitionedDataSet
ReadBPFile(fides::io::DataSetReader* reader,const boost::program_options::variables_map& vm)
{
  vtkm::cont::PartitionedDataSet pds;

  return pds;
}

static void
RunBPBP(const boost::program_options::variables_map& vm)
{
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();
  std::string inputEngineType = "BPFile";
  std::string outputEngineType = "BPFile";

  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();

  int sleepTime = 0;
  if (!vm["sleep"].empty())
  {
    sleepTime = vm["sleep"].as<int>();
  }

  std::cout<<"Run: "<<inputFname<<" "<<inputEngineType<<" --> "<<outputFname<<" "<<outputEngineType<<std::endl;

  fides::io::DataSetReader reader(jsonFile);
  fides::io::DataSetAppendWriter writer(outputFname);

  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  fides::metadata::MetaData selections;

  params["engine_type"] = inputEngineType;
  reader.SetDataSourceParameters("source", params);

  auto metaData = reader.ReadMetaData(paths);
  vtkm::Id totalNumSteps = metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems;

  for (vtkm::Id step = 0; step < totalNumSteps; step++)
  {
    if (sleepTime > 0)
      sleep(sleepTime);
    std::cout<<"Step: "<<step<<std::endl;

    selections.Set(fides::keys::STEP_SELECTION(), fides::metadata::Index(step));

    auto input = reader.ReadDataSet(paths, selections);
    //input.PrintSummary(std::cout);

    auto output = RunService(input, vm);
    writer.Write(output, outputEngineType);
  }
}

static void
RunBPSST(const boost::program_options::variables_map& vm)
{
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();
  std::string inputEngineType = "BPFile";
  std::string outputEngineType = "SST";

  std::cout<<"Run: "<<inputFname<<" "<<inputEngineType<<" --> "<<outputFname<<" "<<outputEngineType<<std::endl;

  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();

  int sleepTime = 0;
  if (!vm["sleep"].empty())
  {
    sleepTime = vm["sleep"].as<int>();
  }

  fides::io::DataSetReader reader(jsonFile);
  fides::io::DataSetAppendWriter writer(outputFname);

  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  fides::metadata::MetaData selections;

  params["engine_type"] = inputEngineType;
  reader.SetDataSourceParameters("source", params);

  auto metaData = reader.ReadMetaData(paths);
  vtkm::Id totalNumSteps = metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems;

  for (vtkm::Id step = 0; step < totalNumSteps; step++)
  {
    std::cout<<"Step: "<<step<<std::endl;
    if (sleepTime > 0)
      sleep(sleepTime);
    selections.Set(fides::keys::STEP_SELECTION(), fides::metadata::Index(step));

    auto input = reader.ReadDataSet(paths, selections);
    //input.PrintSummary(std::cout);

    auto output = RunService(input, vm);
    writer.Write(output, outputEngineType);
  }
//  reader.Close();
  writer.Close();
}

static void
RunSSTBP(const boost::program_options::variables_map& vm)
{
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();
  std::string inputEngineType = "SST";
  std::string outputEngineType = "BPFile";

  std::cout<<"Run: "<<inputFname<<" "<<inputEngineType<<" --> "<<outputFname<<" "<<outputEngineType<<std::endl;

  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();
  int sleepTime = 0;
  if (!vm["sleep"].empty())
  {
    sleepTime = vm["sleep"].as<int>();
  }

  fides::io::DataSetReader reader(jsonFile);
  fides::io::DataSetAppendWriter writer(outputFname);

  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  fides::metadata::MetaData selections;

  params["engine_type"] = inputEngineType;
  reader.SetDataSourceParameters("source", params);

  int step = 0;
  while (true)
  {
    std::cout<<"Step: "<<step<<std::endl;
    if (sleepTime > 0)
      sleep(sleepTime);

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

    auto input = reader.ReadDataSet(paths, selections);
    //input.PrintSummary(std::cout);

    auto output = RunService(input, vm);
    writer.Write(output, outputEngineType);
    step++;
  }
}

static void
RunSSTSST(const boost::program_options::variables_map& vm)
{
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();
  std::string inputEngineType = "SST";
  std::string outputEngineType = "SST";

  std::cout<<"Run: "<<inputFname<<" "<<inputEngineType<<" --> "<<outputFname<<" "<<outputEngineType<<std::endl;

  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();
  int sleepTime = 0;
  if (!vm["sleep"].empty())
  {
    sleepTime = vm["sleep"].as<int>();
  }

  fides::io::DataSetReader reader(jsonFile);
  fides::io::DataSetAppendWriter writer(outputFname);

  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  fides::metadata::MetaData selections;

  params["engine_type"] = inputEngineType;
  reader.SetDataSourceParameters("source", params);

  int step = 0;
  while (true)
  {
    std::cout<<"Step: "<<step<<std::endl;
    if (sleepTime > 0)
      sleep(sleepTime);

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

    auto input = reader.ReadDataSet(paths, selections);
    //input.PrintSummary(std::cout);

    auto output = RunService(input, vm);
    writer.Write(output, outputEngineType);
    step++;
  }
}

static void
RunIT(const boost::program_options::variables_map& vm)
{
  std::string inputEngineType = vm["input_engine"].as<std::string>();
  std::string outputEngineType = vm["output_engine"].as<std::string>();
  if (inputEngineType == "BPFile")
  {
    if (outputEngineType == "BPFile")
      RunBPBP(vm);
    else if (outputEngineType == "SST")
      RunBPSST(vm);
    else
    {
      std::cout<<"ERROR: unknown output engine: "<<outputEngineType<<std::endl;
      return;
    }
  }
  else if (inputEngineType == "SST")
  {
    if (outputEngineType == "BPFile")
      RunSSTBP(vm);
    else if (outputEngineType == "SST")
      RunSSTSST(vm);
    else
    {
      std::cout<<"ERROR: unknown output engine: "<<outputEngineType<<std::endl;
      return;
    }
  }
  else
  {
    std::cout<<"ERROR: unknown input engine: "<<outputEngineType<<std::endl;
    return;
  }
  //RunBPSST(vm);
  //RunSSTBP(vm);
  return;

#if 0
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();

  std::string inputEngineType = "BPFile";
  std::string outputEngineType = "BPFile";
  if (!vm["input_engine"].empty())
    inputEngineType = vm["input_engine"].as<std::string>();
  if (!vm["output_engine"].empty())
    outputEngineType = vm["output_engine"].as<std::string>();    
  
  std::cout<<"Run:: "<<inputFname<< " _"<<inputEngineType<<"_ --> ";
  std::cout<<outputFname<<" _"<<outputEngineType<<"_"<<std::endl;
						      
  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();
  //  else
  //    throw std::runtime_error("Error. SST requires a json file.");

  fides::io::DataSetReader *reader = nullptr;
  if (!jsonFile.empty())
  {
    if (inputEngineType == "BPFile")
      reader = new fides::io::DataSetReader(jsonFile);
    else if (inputEngineType == "SST")
      reader = new fides::io::DataSetReader(jsonFile); //, fides::io::DataSetReader::DataModelInput::JSONFile, true);
  }
  else
  {
    reader = new fides::io::DataSetReader(inputFname, fides::io::DataSetReader::DataModelInput::BPFile);
  }
    
  
  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  fides::metadata::MetaData selections;

  params["engine_type"] = inputEngineType;
  reader->SetDataSourceParameters("source", params);

  fides::io::DataSetAppendWriter writer(outputFname);
  int totalNumSteps = -1;

  bool isBPFile = true;
  if (inputEngineType == "BPFile")
  {
    auto metaData = reader->ReadMetaData(paths);
    isBPFile = true;
    if (metaData.Has(fides::keys::NUMBER_OF_STEPS()))
      totalNumSteps = metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems;
  }
  else
    isBPFile = false;

  int step = 0;
  while (true)
  {
    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;
    if (isBPFile && step == totalNumSteps)
      break;

    //std::cout<<"step= "<<step<<std::endl;
    if (isBPFile)
    {
      selections.Set(fides::keys::STEP_SELECTION(), fides::metadata::Index(step));
    }
    else
    {
      if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;
      auto status = reader->PrepareNextStep(paths);
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
      if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;
    }

    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;

    vtkm::cont::PartitionedDataSet input;

    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;
    if (inputEngineType == "BPFile")
    {
      input = reader->ReadDataSet(paths, selections);
    }
    else
    {
      if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;          
      input = reader->ReadDataSet(paths, selections);
      if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;          
    }
      

    
    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;              
    //std::cout<<"NUMDS: "<<input.GetNumberOfPartitions()<<std::endl;
    //auto ds = input.GetPartition(0);
    //std::cout<<" numCS: "<<ds.GetNumberOfCoordinateSystems()<<std::endl;
    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;              
    auto output = RunService(input, vm);
    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;              
    writer.Write(output, outputEngineType); //"BPFile");

    //fides::metadata::MetaData selections;
    //selections.Set(fides::keys::BLOCK_SELECTION(), blockSelection);

    //auto output = reader.ReadDataSet(paths, selections);
    //std::cout << "Step: " << step << " #ds= " << output.GetNumberOfPartitions() << std::endl;
    //RunService(reader, paths, writer, vm);

    step++;
  }
#endif
}

static void
RunSST2(const boost::program_options::variables_map& vm)
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
      auto output = reader.Read();
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

  //InitDebug();

  namespace po = boost::program_options;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("file", po::value<std::string>(), "Input file")
    ("json", po::value<std::string>(), "Fides JSON data model file")
    ("sleep", po::value<int>(), "Seconds to sleep between reads")
    ("remove-ghost-cells", po::value<std::string>(), "Remove ghost cells from dataset (specify the field name)")
    ("output", po::value<std::string>(), "Output file")
    ("input_engine", po::value<std::string>(), "Adios2 input engine type (BP, SST, or VTK)")
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

  RunIT(vm);
  /*  
  std::string engineType = "BP";
  if (!vm["input_engine"].empty())
    engineType = vm["input_engine"].as<std::string>();

  if (engineType == "SST")
    RunSST(vm);
  else if (engineType == "BP")
    RunBP(vm);
  else if (engineType == "VTK")
    RunVTK(vm);
  else
  {
    std::cout << "Unknown engine type: `" << engineType << "`\n";
    return 1;
  }
  */

#ifdef ENABLE_MPI
  MPI_Finalize();
#endif
  return 0;
}
