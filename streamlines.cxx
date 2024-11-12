#include <mpi.h>
#include <vector>
#include <boost/program_options.hpp>

#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <fides/DataSetReader.h>
#include <fides/DataSetWriter.h>

#include <vtkm/filter/flow/Streamline.h>
#include <vtkm/filter/geometry_refinement/Tube.h>

template <typename T>
static T GetParam(const boost::program_options::variables_map& vm, const char* param)
{
  if (vm[param].empty())
    throw std::runtime_error("Command line parameter " + std::string(param) + " not provided");
  return vm[param].as<T>();
}

template <typename T, vtkm::IdComponent N>
static void String2Vec(const std::string& s, vtkm::Vec<T, N>& vec)
{
  constexpr const char* TOKENS = " ,";
  std::string remaining = s;
  for (vtkm::IdComponent cIndex = 0; cIndex < N; ++cIndex)
  {
    std::size_t pos = remaining.find_first_not_of(TOKENS);
    if (pos == std::string::npos)
    {
      throw std::runtime_error("Cannot convert `" + s + "` to Vec with " + std::to_string(N) +
                               "components.");
    }
    remaining = remaining.substr(pos);
    vec[cIndex] = std::stof(remaining, &pos);
    remaining = remaining.substr(pos);
  }
}

template <typename T>
static T String2Vec(const std::string& s)
{
  T vec;
  String2Vec(s, vec);
  return vec;
}

std::vector<vtkm::Particle> GetSeeds(const boost::program_options::variables_map& vm)
{
  static std::vector<vtkm::Particle> particles;

  if (particles.empty())
  {
    if (vm.count("seed-point") > 0)
      for (auto&& pos_string : vm["seed-point"].as<std::vector<std::string>>())
      {
        particles.emplace_back(String2Vec<vtkm::Vec3f>(pos_string),
                               static_cast<vtkm::Id>(particles.size()));
      }

    // std::cout << "Seeds:\n";
    // for (auto&& p : particles)
    // {
    //   auto pos = p.GetPosition();
    //   std::cout << pos[0] << ", " << pos[1] << ", " << pos[2] << "\n";
    // }

    if (particles.empty())
      throw std::runtime_error("No seed points specified.");
  }

  return particles;
}

static void
RunService(const vtkm::Id& step, xenia::utils::DataSetWriter& writer, vtkm::cont::PartitionedDataSet& pds, const boost::program_options::variables_map& vm)
{
  std::string fieldName = GetParam<std::string>(vm, "field");
  auto seeds = GetSeeds(vm);

  vtkm::filter::flow::Streamline streamline;
  streamline.SetSeeds(seeds, vtkm::CopyFlag::Off);
  streamline.SetStepSize(GetParam<vtkm::FloatDefault>(vm, "step-size"));
  streamline.SetNumberOfSteps(GetParam<vtkm::Id>(vm, "max-steps"));
  streamline.SetActiveField(fieldName);

  auto result = streamline.Execute(pds);

  if (!vm["tube-size"].empty())
  {
    vtkm::filter::geometry_refinement::Tube tubes;
    tubes.SetRadius(vm["tube-size"].as<vtkm::FloatDefault>());
    if (!vm["tube-num-sides"].empty())
      tubes.SetNumberOfSides(vm["tube-num-sides"].as<vtkm::IdComponent>());
    result = tubes.Execute(result);
  }

  std::cout<<"Streamline step: "<<step<<" of "<<fieldName<<std::endl;
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
    auto output = reader.ReadDataSet(step);

    RunService(step, writer, output, vm);
  }
  writer.Close();
}

static void
RunSST(const boost::program_options::variables_map& vm)
{
  std::cout<<"RunSST"<<std::endl;
  xenia::utils::DataSetWriter writer(vm);

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
    RunService(step, writer, output, vm);

    step++;
  }
}

int main(int argc, char** argv)
{
  MPI_Init(NULL, NULL);

  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "Produce this help message.")
    ("file", po::value<std::string>(), "Input Fides bp file.")
    ("json", po::value<std::string>(), "Fides JSON data model file.")
    ("output", po::value<std::string>(), "Output file.")
    ("field", po::value<std::string>(), "field name in input data")
    ("seed-point,s", po::value<std::vector<std::string>>(), "Seed point location. Separate components with spaces or commas. Can be specified multiple times for multiple seeds.")
    ("step-size", po::value<vtkm::FloatDefault>(), "Step size for particle advection.")
    ("max-steps", po::value<vtkm::Id>(), "Maximum number of steps.")
    ("tube-size", po::value<vtkm::FloatDefault>(), "If specified, create tube geometry with the given radius.")
    ("tube-num-sides", po::value<vtkm::IdComponent>(), "Number of sides around tubes (if generated).")
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

  MPI_Finalize();

  #if 0
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();
  std::string fieldName = vm["field"].as<std::string>();
  auto isoVals = vm["isovals"].as<std::vector<vtkm::FloatDefault>>();
  //std::string isoVals = vm["isovals"].as<std::string>();
  std::cout<<inputFname<<" "<<outputFname<<" "<<fieldName<<" "<<isoVals[0]<<std::endl;
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
      auto data = reader.ReadDataSet(step);

      std::cout<<"Step: "<<step<<" num ds= "<<data.GetNumberOfPartitions()<<std::endl;
      vtkm::filter::contour::Contour contour;
      contour.SetGenerateNormals(false);

      contour.SetActiveField(fieldName);
      for (int i = 0; i < isoVals.size(); i++)
        contour.SetIsoValue(i, isoVals[i]);

      vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::Mode::All);
      contour.SetFieldsToPass(selection);

      auto result = contour.Execute(data);

      writer.BeginStep();
      writer.WriteDataSet(result);
      writer.EndStep();
    }
  }
  MPI_Finalize();
#endif


}