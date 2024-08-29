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

static void RenderTest()
{
  vtkm::source::Tangle tangle;
  tangle.SetPointDimensions({ 50, 50, 50 });
  vtkm::cont::DataSet tangleData = tangle.Execute();
  tangleData.PrintSummary(std::cout);

  std::string fieldName = "tangle";

  // Set up a camera for rendering the input data
  vtkm::rendering::Camera camera;
  camera.SetLookAt(vtkm::Vec3f_32(0.5, 0.5, 0.5));
  camera.SetViewUp(vtkm::make_Vec(0.f, 1.f, 0.f));
  camera.SetClippingRange(1.f, 10.f);
  camera.SetFieldOfView(60.f);
  camera.SetPosition(vtkm::Vec3f_32(1.5, 1.5, 1.5));

  camera.SetLookAt(vtkm::Vec3f_32(3.5, 3.5, 3.5));
    camera.SetViewUp(vtkm::make_Vec(0.f, 1.f, 0.f));
    camera.SetClippingRange(1.f, 50.f);
    camera.SetFieldOfView(60.f);
  camera.SetPosition(vtkm::Vec3f_32(10,10,10));


  vtkm::cont::ColorTable colorTable("inferno");

  // Background color:
  vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
  vtkm::rendering::Actor actor(tangleData.GetCellSet(),
                               tangleData.GetCoordinateSystem(),
                               tangleData.GetField(fieldName),
                               colorTable);
  vtkm::rendering::Scene scene;
  scene.AddActor(actor);
  // 2048x2048 pixels in the canvas:
  CanvasRayTracer canvas(2048, 2048);
  // Create a view and use it to render the input data using OS Mesa

  vtkm::rendering::View3D view(scene, MapperVolume(), canvas, camera, bg);
  view.Paint();
  view.SaveAs("xxxx-test-volume.png");

  // Compute an isosurface:
  vtkm::filter::contour::Contour filter;
  // [min, max] of the tangle field is [-0.887, 24.46]:
  filter.SetIsoValue(3.0);
  filter.SetActiveField(fieldName);
  vtkm::cont::DataSet isoData = filter.Execute(tangleData);
  // Render a separate image with the output isosurface
  vtkm::rendering::Actor isoActor(
    isoData.GetCellSet(), isoData.GetCoordinateSystem(), isoData.GetField(fieldName), colorTable);
  // By default, the actor will automatically scale the scalar range of the color table to match
  // that of the data. However, we are coloring by the scalar that we just extracted a contour
  // from, so we want the scalar range to match that of the previous image.
  isoActor.SetScalarRange(actor.GetScalarRange());
  vtkm::rendering::Scene isoScene;
  isoScene.AddActor(std::move(isoActor));

  // Wireframe surface:
  vtkm::rendering::View3D isoView(isoScene, MapperWireframer(), canvas, camera, bg);
  isoView.Paint();
  isoView.SaveAs("xxx-test-isosurface_wireframer.png");

  // Smooth surface:
  vtkm::rendering::View3D solidView(isoScene, MapperRayTracer(), canvas, camera, bg);
  solidView.Paint();
  solidView.SaveAs("xxx-test-isosurface_raytracer.png");
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
  RenderTest();


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