#include <mpi.h>
#include <string>
#include <vector>
#include <iostream>

#include "utils/CommandLineArgParser.h"
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
MakeCanvas(const xenia::utils::CommandLineArgParser& args)
{
  vtkm::Vec<vtkm::Id,2> res(256, 256);

  if (args.HasArg("--imagesize"))
  {
    res[0] = std::stoi(args.GetArg("--imagesize")[0]);
    res[1] = std::stoi(args.GetArg("--imagesize")[1]);
  }

  return vtkm::rendering::CanvasRayTracer(res[0], res[1]);
}

vtkm::rendering::Camera
MakeCamera(const xenia::utils::CommandLineArgParser& args)
{
  vtkm::rendering::Camera camera;
  vtkm::Vec3f_32 position(0,0,10);
  vtkm::Vec3f_32 lookAt(0,0,0);
  vtkm::Vec3f_32 up(0,1,0);
  vtkm::FloatDefault fov = 60;
  vtkm::Vec2f_32 clip(1.0, 10.0);

  if (args.HasArg("--position"))
  {
    const auto& arg = args.GetArg("--position");
    for (int i = 0; i < 3; i++)
      position[i] = std::stof(arg[i]);
  }
  if (args.HasArg("--lookat"))
  {
    const auto& arg = args.GetArg("--lookat");
    for (int i = 0; i < 3; i++)
      lookAt[i] = std::stof(arg[i]);
  }

  if (args.HasArg("--up"))
  {
    const auto& arg = args.GetArg("--up");
    for (int i = 0; i < 3; i++)
      up[i] = std::stof(arg[i]);
  }
  if (args.HasArg("--fov"))
  {
    const auto& arg = args.GetArg("--fov");
    fov = std::stof(arg[0]);
  }
  if (args.HasArg("--clip"))
  {
    const auto& arg = args.GetArg("--clip");
    for (int i = 0; i < 2; i++)
      clip[i] = std::stof(arg[i]);
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
  MPI_Init(&argc, &argv);

  xenia::utils::CommandLineArgParser args(argc, argv, {"--file", "--output", "--field"});

  auto data = xenia::utils::ReadData(args);
  std::string fieldName = args.GetArg("--field")[0];

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

  auto canvas = MakeCanvas(args);
  auto camera = MakeCamera(args);

  vtkm::rendering::View3D view(scene, vtkm::rendering::MapperRayTracer(), canvas, camera, bg);

  view.Paint();
  view.SaveAs(args.GetArg("--output")[0]);

  MPI_Finalize();
  return 0;
}
