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
  std::cout<<"Opening: "<<inputFname<<std::endl;

  //There is an issue with the DataSetWriter. When it goes out of scope, it calls Engine.Close().
  //This makes some MPI calls. When the object destructs after MPI_Finalize, results in an error.
  {
    xenia::utils::DataSetReader reader(vm);
    xenia::utils::DataSetWriter writer(vm);
    reader.Init();

    vtkm::Id numSteps = reader.GetNumSteps();
    for (vtkm::Id step = 0; step < numSteps; step++)
    {
      reader.Step = step;
      auto output = reader.ReadDataSet(step);

      std::cout<<"Step: "<<step<<" num ds= "<<output.GetNumberOfPartitions()<<std::endl;
      writer.BeginStep();
      writer.WriteDataSet(output);
      writer.EndStep();
    }
  }

  MPI_Finalize();
  return 0;
}
