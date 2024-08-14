#include <fides/DataSetReader.h>
#include <mpi.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils/CommandLineArgParser.h"

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  if (vm.count("help")) {
      cout << desc << "\n";
      return 1;
  }
  
  


  if (vm.count("compression"))
      cout << "Compression level was set to " 
   << vm["compression"].as<int>() << ".\n";
  else
      cout << "Compression level was not set.\n";


  xenia::utils::DataSetReader reader(vm);
  reader.BeginStep();
  auto data = reader.ReadDataSet();
  reader.EndStep();

  data.PrintSummary(std::cout);

  MPI_Finalize();
  return 0;
}
