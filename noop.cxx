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
  std::cout<<__LINE__<<std::endl;
  MPI_Init(NULL, NULL);
  std::cout<<__LINE__<<std::endl;

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
  std::cout<<"Opening: "<<inputFname<<std::endl;

  xenia::utils::DataSetReader reader(vm);
  xenia::utils::DataSetWriter writer(vm);
  reader.Init();
      
  reader.Step = 0;
  while(true)
  {
    auto status = reader.BeginStep();
    if (status == fides::StepStatus::NotReady)
      continue;
    else if (status == fides::StepStatus::EndOfStream)
      break;

    auto output = reader.FidesReader->ReadDataSet(reader.Paths, reader.MetaData);

    std::cout<<"Step: "<<reader.Step<<" num ds= "<<output.GetNumberOfPartitions()<<std::endl;

    writer.BeginStep();
    writer.WriteDataSet(output);
    writer.EndStep();
    
   reader.EndStep();
  }

  MPI_Finalize();
  return 0;
}
