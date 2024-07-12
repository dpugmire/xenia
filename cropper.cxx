#include <mpi.h>
#include <string>
#include <vector>

#include "utils/CommandLineArgParser.h"
#include "utils/ReadData.h"
#include "utils/WriteData.h"

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  xenia::utils::CommandLineArgParser args(argc, argv, {"--file", "--output", "--index" });

  auto data = xenia::utils::ReadData(args);
  int index = std::stoi(args.GetArg("--index")[0]);

  if (index < data.GetNumberOfPartitions())
    xenia::utils::WriteData(data, args.GetArg("--output")[0]);
  else
    std::cerr<<"Error: index out of range."<<std::endl;

  MPI_Finalize();
  return 0;
}
