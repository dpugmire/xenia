#include <mpi.h>
#include <string>
#include <vector>
#include <iostream>

#include <boost/program_options.hpp>
#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>

vtkm::rendering::CanvasRayTracer
MakeCanvas(boost::program_options::variables_map& vm)
{
  vtkm::Vec<vtkm::Id,2> res(256, 256);

  if (vm.count("imagesize") == 2)
  {
    const auto& vals = vm["imagesize"].as<std::vector<vtkm::FloatDefault>>();
    res[0] = vals[0];
    res[1] = vals[1];
  }

  return vtkm::rendering::CanvasRayTracer(res[0], res[1]);
}

vtkm::rendering::Camera
MakeCamera(boost::program_options::variables_map& vm)
{
  vtkm::rendering::Camera camera;
  vtkm::Vec3f_32 position(1.5, 1.5, 1.5);
  vtkm::Vec3f_32 lookAt(.5, .5, .5);
  vtkm::Vec3f_32 up(0,1,0);
  vtkm::FloatDefault fov = 60;
  vtkm::Vec2f_32 clip(1.0, 10.0);

  if (vm.count("position") == 3)
  {
    const auto& vals = vm["position"].as<std::vector<vtkm::FloatDefault>>();
    for (int i = 0; i < 3; i++)
      position[i] = vals[i];
  }
  if (vm.count("lookat") == 3)
  {
    const auto& vals = vm["lookat"].as<std::vector<vtkm::FloatDefault>>();
    for (int i = 0; i < 3; i++)
      lookAt[i] = vals[i];
  }
  if (vm.count("up") == 3)
  {
    const auto& vals = vm["up"].as<std::vector<vtkm::FloatDefault>>();
    for (int i = 0; i < 3; i++)
      up[i] = vals[i];
  }
  if (vm.count("fov") == 1)
  {
    fov = vm["fov"].as<vtkm::FloatDefault>();
  }
  if (vm.count("clip") == 2)
  {
    const auto& vals = vm["clip"].as<std::vector<vtkm::FloatDefault>>();
    clip[0] = vals[0];
    clip[1] = vals[1];
  }

  camera.SetPosition(position);
  camera.SetLookAt(lookAt);
  camera.SetViewUp(up);
  camera.SetFieldOfView(fov);
  camera.SetClippingRange(clip[0], clip[1]);

  return camera;
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
    ("position", po::value<std::vector<std::string>>()->multitoken(), "Camera position")
    ("lookat", po::value<std::vector<std::string>>()->multitoken(), "Camera look at position")
    ("up", po::value<std::vector<std::string>>()->multitoken(), "Camera up direction")
    ("fov", po::value<float>(), "Camera up direction")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  
  if (vm.count("help"))
  {
    std::cout << desc << "\n";
    return 1;
  }

  xenia::utils::DataSetReader reader(vm);
  reader.BeginStep();
  auto data = reader.ReadDataSet();
  reader.EndStep();
  std::string fieldName = vm["field"].as<std::string>();

  vtkm::cont::ColorTable colorTable("inferno");
  vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
  
  vtkm::rendering::Scene scene;
  for (const auto& ds : data)
  {
    vtkm::rendering::Actor actor(ds.GetCellSet(),
                                 ds.GetCoordinateSystem(),
                                 ds.GetField(fieldName),
                                 colorTable);
    scene.AddActor(actor);
  }

  std::string output = vm["output"].as<std::string>();
  auto canvas = MakeCanvas(vm);
  auto camera = MakeCamera(vm);
  
  vtkm::rendering::View3D view(scene, vtkm::rendering::MapperRayTracer(), canvas, camera, bg);

  view.Paint();
  view.SaveAs(output);
  
  MPI_Finalize();
  return 0;
}
