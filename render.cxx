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

vtkm::rendering::CanvasRayTracer
MakeCanvas(boost::program_options::variables_map& vm)
{
  vtkm::Vec<vtkm::Id,2> res(512, 512);//1024, 1024);//256, 256);

  if (vm.count("imagesize") == 2)
  {
    const auto& vals = vm["imagesize"].as<std::vector<vtkm::FloatDefault>>();
    res[0] = vals[0];
    res[1] = vals[1];
  }

  auto canvas =vtkm::rendering::CanvasRayTracer(res[0], res[1]);
  return canvas;
}

vtkm::rendering::Camera
MakeCamera(boost::program_options::variables_map& vm)
{
  vtkm::rendering::Camera camera;
  vtkm::Vec3f_32 position(10,3,3); //(1.5, 1.5, 1.5);
  vtkm::Vec3f_32 lookAt(3,3,3); //(.5, .5, .5);
  vtkm::Vec3f_32 up(0,1,0);
  vtkm::FloatDefault fov = 60;
  vtkm::Vec2f_32 clip(-10.0, 10.0);

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

  std::cout<<"Pos: "<<position<<std::endl;
  std::cout<<"LookAt: "<<lookAt<<std::endl;
  std::cout<<"Up: "<<up<<std::endl;
  std::cout<<"Fov: "<<fov<<std::endl;
  std::cout<<"clip "<<clip<<std::endl;
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
    ("position", po::value<std::vector<float>>()->multitoken(), "Camera position")
    ("lookat", po::value<std::vector<float>>()->multitoken(), "Camera look at position")
    ("up", po::value<std::vector<float>>()->multitoken(), "Camera up direction")
    ("fov", po::value<float>(), "Camera up direction")
    ("clip", po::value<std::vector<float>>()->multitoken(), "Clipping range")
    ;

  for (int i = 0; i < argc; i++)
  std::cout<<"Arg_"<<i<<"  :"<<argv[i]<<":"<<std::endl;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << desc << "\n";
    return 1;
  }

  xenia::utils::DataSetReader reader(vm);
  xenia::utils::DataSetWriter writer(vm);
  reader.Init();

  std::string fieldName = vm["field"].as<std::string>();

  vtkm::cont::ColorTable colorTable("inferno");
  vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
  std::string output = vm["output"].as<std::string>();

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

    std::string output = vm["output"].as<std::string>();

    vtkm::rendering::View3D view(scene, vtkm::rendering::MapperRayTracer(), canvas, camera, bg);

    view.Paint();
    view.SaveAs(output);
  }

  MPI_Finalize();
}