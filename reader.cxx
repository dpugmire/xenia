#include <fides/DataSetReader.h>
#include <mpi.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils/CommandLineArgParser.h"

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);

  xenia::utils::CommandLineArgParser args(argc, argv, {"--json", "--file"});

  auto jsonFile = args.GetArg("--json")[0];
  auto bpFile = args.GetArg("--file")[0];

  fides::io::DataSetReader reader(jsonFile);
  std::unordered_map<std::string, std::string> paths;

  paths["source"] = std::string(bpFile);

  auto metaData = reader.ReadMetaData(paths);
  auto data = reader.ReadDataSet(paths, metaData);
  data.PrintSummary(std::cout);

  MPI_Finalize();
  return 0;
}
