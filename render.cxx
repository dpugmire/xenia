#include <mpi.h>
#include <string>
#include <vector>
#include <iostream>

#include <boost/program_options.hpp>
#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <vtkm/cont/Initialize.h>
#include <vtkm/source/Tangle.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/filter/contour/Contour.h>

using vtkm::rendering::CanvasRayTracer;
using vtkm::rendering::MapperRayTracer;
using vtkm::rendering::MapperVolume;
using vtkm::rendering::MapperWireframer;

std::string
CreateOutputFileName(const std::string& fname, vtkm::Id step)
{
 if (fname.find('%') != std::string::npos)
 {
   char buffer[128];
   snprintf(buffer, sizeof(buffer), fname.c_str(), step);
   std::string outFname(buffer);
   return outFname;
 }
 else
   return fname;
}

vtkm::rendering::CanvasRayTracer
MakeCanvas(const boost::program_options::variables_map& vm)
{
  vtkm::Vec<vtkm::Id,2> res(1024, 1024);

  if (!vm["imagesize"].empty())
  {
    const auto& vals = vm["imagesize"].as<std::vector<int>>();
    res[0] = static_cast<vtkm::Id>(vals[0]);
    res[1] = static_cast<vtkm::Id>(vals[1]);
  }

  auto canvas =vtkm::rendering::CanvasRayTracer(res[0], res[1]);
  return canvas;
}

vtkm::rendering::Camera
MakeCamera(const boost::program_options::variables_map& vm)
{
  vtkm::rendering::Camera camera;
  vtkm::Vec3f_32 position(1.5, 1.5, 1.5);
  vtkm::Vec3f_32 lookAt(.5, .5, .5);
  vtkm::Vec3f_32 up(0,1,0);
  vtkm::FloatDefault fov = 60;
  vtkm::Vec2f_32 clip(-1.0, 1.0);

  if (!vm["position"].empty())
  {
    const auto& vals = vm["position"].as<std::vector<float>>();
    for (std::size_t i = 0; i < 3; i++)
      position[i] = vals[i];
  }

  if (!vm["lookat"].empty())
  {
    const auto& vals = vm["lookat"].as<std::vector<float>>();
    for (int i = 0; i < 3; i++)
      lookAt[i] = vals[i];
  }
  if (!vm["up"].empty())
  {
    const auto& vals = vm["up"].as<std::vector<float>>();
    for (int i = 0; i < 3; i++)
      up[i] = vals[i];
  }
  if (!vm["fov"].empty())
  {
    fov = vm["fov"].as<float>();
  }
  if (!vm["clip"].empty())
  {
    const auto& vals = vm["clip"].as<std::vector<vtkm::FloatDefault>>();
    clip[0] = vals[0];
    clip[1] = vals[1];
  }

/*
  std::cout<<"Pos: "<<position<<std::endl;
  std::cout<<"LookAt: "<<lookAt<<std::endl;
  std::cout<<"Up: "<<up<<std::endl;
  std::cout<<"Fov: "<<fov<<std::endl;
  std::cout<<"clip "<<clip<<std::endl;
*/
  camera.SetPosition(position);
  camera.SetLookAt(lookAt);
  camera.SetViewUp(up);
  camera.SetFieldOfView(fov);
  camera.SetClippingRange(clip[0], clip[1]);

  return camera;
}

static void
RunService(const vtkm::Id& step, xenia::utils::DataSetWriter& writer, vtkm::cont::PartitionedDataSet& data, const boost::program_options::variables_map& vm)
{
  std::string outputFile = vm["output"].as<std::string>();

  auto canvas = MakeCanvas(vm);
  auto camera = MakeCamera(vm);
  std::string fieldName = vm["field"].as<std::string>();

  vtkm::cont::ColorTable colorTable("inferno");
  vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);

  vtkm::Range scalarRange(0.0, 1.0);
  if (!vm["scalar_range"].empty())
  {
    const auto& vals = vm["scalar_range"].as<std::vector<float>>();
    scalarRange.Min = vals[0];
    scalarRange.Max = vals[1];
  }


  vtkm::rendering::Scene scene;
    for (const auto& ds : data)
    {
      vtkm::rendering::Actor actor(ds.GetCellSet(),
                                  ds.GetCoordinateSystem(),
                                  ds.GetField(fieldName),
                                  colorTable);
      actor.SetScalarRange(scalarRange);
      scene.AddActor(actor);
    }

    vtkm::rendering::View3D view(scene, vtkm::rendering::MapperRayTracer(), canvas, camera, bg);

    view.Paint();
    auto fname = CreateOutputFileName(outputFile, step);
    std::cout<<"Render step: "<<step<<" to "<<fname<<std::endl;
    view.SaveAs(fname);
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
    reader.Step = step;
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
    ("help", "produce help message")
    ("file", po::value<std::string>(), "Input file")
    ("json", po::value<std::string>(), "Fides JSON data model file")
    ("output", po::value<std::string>(), "Output file")
    ("field", po::value<std::string>(), "field name in input data")
    ("position", po::value<std::vector<float>>()->multitoken(), "Camera position")
    ("lookat", po::value<std::vector<float>>()->multitoken(), "Camera look at position")
    ("up", po::value<std::vector<float>>()->multitoken(), "Camera up direction")
    ("fov", po::value<float>(), "Camera up direction")
    ("clip", po::value<std::vector<float>>()->multitoken(), "Clipping range")
    ("imagesize", po::value<std::vector<int>>()->multitoken(), "Image size")
    ("scalar_range", po::value<std::vector<float>>()->multitoken(), "Scalar rendering range")
    ("input_engine", po::value<std::string>(), "Adios2 input engine type (BP or SST")
    ("output_engine", po::value<std::string>(), "Adios2 output engine type (BP or SST")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  std::string engineType = "BP5";
  if (!vm["input_engine"].empty())
    engineType = vm["input_engine"].as<std::string>();

  if (engineType == "SST")
    RunSST(vm);
  else
    RunBP(vm);

  MPI_Finalize();

  #if 0

  xenia::utils::DataSetReader reader(vm);
  xenia::utils::DataSetWriter writer(vm);
  reader.Init();

  std::string fieldName = vm["field"].as<std::string>();

  vtkm::cont::ColorTable colorTable("inferno");
  vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
  std::string outputFile = vm["output"].as<std::string>();



  auto canvas = MakeCanvas(vm);
  auto camera = MakeCamera(vm);

  vtkm::Id numSteps = reader.GetNumSteps();
  for (vtkm::Id step = 0; step < numSteps; step++)
  {
    reader.Step = step;
    auto data = reader.ReadDataSet(step);
    std::cout<<"Step: "<<step<<" num ds= "<<data.GetNumberOfPartitions()<<std::endl;

    vtkm::rendering::Scene scene;
    for (const auto& ds : data)
    {
      vtkm::rendering::Actor actor(ds.GetCellSet(),
                                  ds.GetCoordinateSystem(),
                                  ds.GetField(fieldName),
                                  colorTable);
      scene.AddActor(actor);
    }

    vtkm::rendering::View3D view(scene, vtkm::rendering::MapperRayTracer(), canvas, camera, bg);

    view.Paint();
    auto fname = CreateOutputFileName(outputFile, step);
    view.SaveAs(fname);
  }

  MPI_Finalize();
  #endif
}