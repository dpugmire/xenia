#include <mpi.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <regex>

#include "utils/CommandLineArgParser.h"
#include "utils/ReadData.h"
#include "utils/WriteData.h"

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  xenia::utils::CommandLineArgParser args(argc, argv, {"--file", "--output"});

  auto data = xenia::utils::ReadData(args);

  xenia::utils::WriteData(data, args.GetArg("--output")[0]);

  MPI_Finalize();
  return 0;
}
