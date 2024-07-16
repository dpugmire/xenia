#include <mpi.h>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

#include "utils/CommandLineArgParser.h"
#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <fides/DataSetReader.h>


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

  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();

  xenia::utils::DataSetReader reader(vm);
      
  int step = 0;
  while(true)
  {
    auto status = reader.FidesReader->PrepareNextStep(reader.Paths);
    if (status == fides::StepStatus::NotReady)
      continue;
    else if (status == fides::StepStatus::EndOfStream)
      break;

  if (step == 0)
    reader.Init();


    auto output = reader.FidesReader->ReadDataSet(reader.Paths, reader.MetaData);

    std::cout<<"Step: "<<step<<" num ds= "<<output.GetNumberOfPartitions()<<std::endl;
    step++;

    xenia::utils::WriteData(output, vm["output"].as<std::string>());
  }

  MPI_Finalize();
  return 0;
}
