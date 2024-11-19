#include <mpi.h>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <unistd.h> // for sleep()

#include "utils/Debug.h"
#include "utils/CommandLineArgParser.h"
#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/CellClassification.h>
#include <fides/DataSetReader.h>

#include <vtkm/filter/contour/Contour.h>
#include <vtkm/filter/field_conversion/PointAverage.h>
#include <vtkm/filter/field_conversion/CellAverage.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/filter/contour/Contour.h>

#include <vtkm/filter/field_transform/CompositeVectors.h>
#include <vtkm/filter/flow/Streamline.h>
#include <vtkm/filter/geometry_refinement/Tube.h>

using vtkm::rendering::CanvasRayTracer;
using vtkm::rendering::MapperRayTracer;
using vtkm::rendering::MapperVolume;
using vtkm::rendering::MapperWireframer;

static std::string
CreateVisItFile(const std::string& outputFileName, int totalNumDS, int step)
{
  auto pos = outputFileName.find(".vtk");
  auto VisItFileName = outputFileName;
  std::string pattern(".visit");
  VisItFileName.replace(pos, pattern.size(), pattern);

  if (step == 0)
  {
    std::ofstream fout(VisItFileName);
    fout<<"!NBLOCKS "<<totalNumDS<<std::endl;
    fout.close();
  }

  return VisItFileName;
}
std::vector<std::string>
GetVTKOutputFileNames(std::string& outputFileName, int timestep, int totalNumDS, int blk0, int blk1)
{
  std::vector<std::string> outputFileNames;

  std::string fname;

  auto pos = outputFileName.find(".vtk");
  std::string pattern(".ts_%d_ds_%d.vtk");
  outputFileName.replace(pos, pattern.size(), pattern);
  char buffer[128];
  for (int i = blk0; i < blk1; i++)
  {
      snprintf(buffer, sizeof(buffer), outputFileName.c_str(), timestep, i);
      outputFileNames.push_back(buffer);
  }

  return outputFileNames;
}

static void
AppendVTKFiles(const std::string& visitFileName, const std::vector<std::string>& fileNames)
{
  auto fout = std::ofstream(visitFileName, std::ios::app);
  for (const auto& fileName : fileNames)
      fout<<fileName<<std::endl;

  fout.close();
}

static bool
WriteVTK(const vtkm::cont::PartitionedDataSet& pds,
         int step,
         const boost::program_options::variables_map& vm)
{
  std::cout<<"WriteVTK: step= "<<step<<std::endl;
  std::string outputFileName = vm["vtkfile"].as<std::string>();

  int localNumDS = static_cast<int>(pds.GetNumberOfPartitions());
  int totalNumDS = localNumDS;
  int b0 = 0, b1 = localNumDS;

  if (totalNumDS == 0)
      return false;

  auto visitFileName = CreateVisItFile(outputFileName, totalNumDS, step);
  auto outputFileNames = GetVTKOutputFileNames(outputFileName, step, totalNumDS, 0, totalNumDS);
  AppendVTKFiles(visitFileName, outputFileNames);

  int blkIdx = 0;
  for (const auto& ds : pds.GetPartitions())
  {
      vtkm::io::VTKDataSetWriter writer(outputFileNames[blkIdx]);
      writer.WriteDataSet(ds);
      blkIdx++;
  }

  return true;
}

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
  std::string outputFile = vm["output"].as<std::string>();

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

static const std::vector<std::string>& GetComponentFieldList(const boost::program_options::variables_map& vm)
{
  static std::vector<std::string> componentNames;

  if (componentNames.empty())
  {
    componentNames.reserve(3);
    for (const std::string& axisName : { "x", "y", "z" })
    {
      std::string argname = "field" + axisName;
      if (!vm[argname].empty())
      {
        componentNames.push_back(vm[argname].as<std::string>());
      }
    }
  }

  return componentNames;
}

static const std::vector<vtkm::Particle>& GetSeeds(const boost::program_options::variables_map& vm)
{
  static std::vector<vtkm::Particle> particles;

  if (particles.empty())
  {
    if (!vm["seed-grid-bounds"].empty())
    {
      auto b =
        String2Vec<vtkm::Vec<vtkm::Float64, 6>>(vm["seed-grid-bounds"].as<std::string>());
      vtkm::Bounds bounds{ b[0], b[1], b[2], b[3], b[4], b[5] };
      vtkm::IdComponent3 dims{ 10, 10, 10 };
      if (!vm["seed-grid-dims"].empty())
      {
        String2Vec(vm["seed-grid-dims"].as<std::string>(), dims);
      }

      particles.reserve(dims[0] * dims[1] * dims[2]);

      auto minCorner = bounds.MinCorner();
      auto spacing = (bounds.MaxCorner() - minCorner) / static_cast<vtkm::Vec3f_64>(dims);
      for (vtkm::IdComponent zIndex = 0; zIndex < dims[2]; ++zIndex)
        for (vtkm::IdComponent yIndex = 0; yIndex < dims[1]; ++yIndex)
          for (vtkm::IdComponent xIndex = 0; xIndex < dims[0]; ++xIndex)
            particles.emplace_back(vtkm::Vec3f_64(xIndex, yIndex, zIndex) * spacing + minCorner,
                                   static_cast<vtkm::Id>(particles.size()));
    }
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

static vtkm::cont::PartitionedDataSet
RunService(int step,
           const vtkm::cont::PartitionedDataSet& input,
	         const boost::program_options::variables_map& vm)
{
  auto serviceType = vm["service"].as<std::string>();

  vtkm::cont::PartitionedDataSet output;
  if (serviceType == "copier")
  {
    std::cout<<"Copier: step= "<<step<<std::endl;
    output = input;
  }
  else if (serviceType == "converter")
  {
    WriteVTK(input, step, vm);
    output = input;
  }
  else if (serviceType == "contour")
  {
    vtkm::cont::PartitionedDataSet input2 = input;
    std::cout<<"Contour: step= "<<step<<std::endl;
    std::string fieldName = vm["field"].as<std::string>();
    auto isoVals = vm["isovals"].as<std::vector<vtkm::FloatDefault>>();

    if (!vm["cell_to_point"].empty())
    {
      vtkm::filter::field_conversion::PointAverage avg;
      //avg.SetActiveField(fieldName, vtkm::cont::Field::Association::Cells);
      avg.SetActiveField(fieldName);
      fieldName = fieldName + "_point";
      avg.SetOutputFieldName(fieldName);
      input2 = avg.Execute(input2);
    }

    vtkm::filter::contour::Contour contour;
    contour.SetGenerateNormals(false);

    contour.SetActiveField(fieldName);
    for (int i = 0; i < isoVals.size(); i++)
      contour.SetIsoValue(i, isoVals[i]);

    vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::Mode::All);
    contour.SetFieldsToPass(selection);

    output = contour.Execute(input2);
  }
  else if (serviceType == "streamlines")
  {
  std::cout<<__LINE__<<std::endl;
    vtkm::cont::PartitionedDataSet input2 = input;

    std::string fieldName;
    if (!vm["field"].empty())
    {
      fieldName = vm["field"].as<std::string>();
    } else if (!vm["fieldx"].empty()) {
      vtkm::filter::field_transform::CompositeVectors combineVec;
      combineVec.SetFieldNameList(GetComponentFieldList(vm));
      combineVec.SetOutputFieldName("_xenia_vec_");

      input2 = combineVec.Execute(input);
      fieldName = combineVec.GetOutputFieldName();
    } else {
      throw std::runtime_error(
        "Must provide either `--field` or `--fieldx`, `--fieldy`, and `--fieldz` arguments.");
    }
std::cout<<__LINE__<<std::endl;

    auto seeds = GetSeeds(vm);
std::cout<<__LINE__<<std::endl;

    vtkm::filter::flow::Streamline streamline;
    streamline.SetSeeds(seeds, vtkm::CopyFlag::Off);
    streamline.SetStepSize(GetParam<vtkm::FloatDefault>(vm, "step-size"));
    streamline.SetNumberOfSteps(GetParam<vtkm::Id>(vm, "max-steps"));
    streamline.SetActiveField(fieldName);

    output = streamline.Execute(input2);

    if (!vm["tube-size"].empty())
    {
      vtkm::filter::geometry_refinement::Tube tubes;
      tubes.SetRadius(vm["tube-size"].as<vtkm::FloatDefault>());
      if (!vm["tube-num-sides"].empty())
        tubes.SetNumberOfSides(vm["tube-num-sides"].as<vtkm::IdComponent>());
      output = tubes.Execute(output);

      //Add field to tubes.
      for (vtkm::Id i = 0; i < output.GetNumberOfPartitions(); i++)
      {
        auto ds = output.GetPartition(i);
        vtkm::Id npts = ds.GetNumberOfPoints();
        std::vector<vtkm::FloatDefault> scalars(npts, 1.0);
        ds.AddPointField("scalar", scalars);
        output.ReplacePartition(i, ds);
      }

    }
  }
  else if (serviceType == "render")
  {
    std::string outputFile = vm["output"].as<std::string>();

    auto canvas = MakeCanvas(vm);
    auto camera = MakeCamera(vm);
    std::string fieldName = "";
    if (!vm["field"].empty())
      fieldName = vm["field"].as<std::string>();

    //use the raytracer.
    if (!fieldName.empty())
    {
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
        for (const auto& ds : input)
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
    else //wireframe mapper
    {
    }
  }
  else
  {
    throw std::runtime_error("Error: Unknown service " + serviceType);
  }

  return output;
}

static void
RunService2(xenia::utils::DataSetWriter& writer, const vtkm::cont::PartitionedDataSet& pds, const boost::program_options::variables_map& /*vm*/)
{
  writer.BeginStep();
  writer.WriteDataSet(pds);
  writer.EndStep();
}

static void
RunVTK(const boost::program_options::variables_map& vm)
{
  std::cout << "RunVTK" << std::endl;
  vtkm::io::VTKDataSetReader reader(vm["file"].as<std::string>());
  xenia::utils::DataSetWriter writer(vm);

  auto output = reader.ReadDataSet();

  RunService2(writer, vtkm::cont::PartitionedDataSet{ output }, vm);

  writer.Close();
}

static void
RunBP(const boost::program_options::variables_map& vm)
{
  int rank = 0, numProcs = 1;
#ifdef ENABLE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
#endif
  std::cout<<"RunBP"<<std::endl;
  xenia::utils::DataSetReader reader(vm);
  xenia::utils::DataSetWriter writer(vm);
  reader.Init();

  vtkm::Id numSteps = reader.GetNumSteps();

  for (vtkm::Id step = 0; step < numSteps; step++)
  {
    reader.BeginStep();
    auto output = reader.Read();
    std::cout<<rank<<": has "<<output.GetNumberOfPartitions()<<std::endl;

    RunService2(writer, output, vm);

    reader.EndStep();
  }

  writer.Close();
}

vtkm::cont::PartitionedDataSet
ReadBPFile(fides::io::DataSetReader* reader,const boost::program_options::variables_map& vm)
{
  vtkm::cont::PartitionedDataSet pds;

  return pds;
}

static void
RunBPBP(const boost::program_options::variables_map& vm)
{
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();
  std::string inputEngineType = "BPFile";
  std::string outputEngineType = "BPFile";

  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();

  int sleepTime = 0;
  if (!vm["sleep"].empty())
  {
    sleepTime = vm["sleep"].as<int>();
  }

  std::cout<<"Run: "<<inputFname<<" "<<inputEngineType<<" --> "<<outputFname<<" "<<outputEngineType<<std::endl;

  fides::io::DataSetReader reader(jsonFile);
  fides::io::DataSetAppendWriter writer(outputFname);

  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  fides::metadata::MetaData selections;

  params["engine_type"] = inputEngineType;
  reader.SetDataSourceParameters("source", params);

  auto metaData = reader.ReadMetaData(paths);
  vtkm::Id totalNumSteps = metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems;

  for (vtkm::Id step = 0; step < totalNumSteps; step++)
  {
    if (sleepTime > 0)
      sleep(sleepTime);
    std::cout<<"Step: "<<step<<std::endl;

    selections.Set(fides::keys::STEP_SELECTION(), fides::metadata::Index(step));

    auto input = reader.ReadDataSet(paths, selections);
    auto output = RunService(step, input, vm);
    if (output.GetNumberOfPartitions() > 0)
      writer.Write(output, outputEngineType);
  }
}

static void
RunBPSST(const boost::program_options::variables_map& vm)
{
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();
  std::string inputEngineType = "BPFile";
  std::string outputEngineType = "SST";

  std::cout<<"Run: "<<inputFname<<" "<<inputEngineType<<" --> "<<outputFname<<" "<<outputEngineType<<std::endl;

  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();

  int sleepTime = 0;
  if (!vm["sleep"].empty())
  {
    sleepTime = vm["sleep"].as<int>();
  }

  fides::io::DataSetReader reader(jsonFile);
  fides::io::DataSetAppendWriter writer(outputFname);

  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  fides::metadata::MetaData selections;

  params["engine_type"] = inputEngineType;
  reader.SetDataSourceParameters("source", params);

  auto metaData = reader.ReadMetaData(paths);
  vtkm::Id totalNumSteps = metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems;

  std::cout<<"JSON= "<<jsonFile<<std::endl;
  for (vtkm::Id step = 0; step < totalNumSteps; step++)
  {
    std::cout<<"Step: "<<step<<std::endl;
    if (sleepTime > 0)
      sleep(sleepTime);
    selections.Set(fides::keys::STEP_SELECTION(), fides::metadata::Index(step));

    auto input = reader.ReadDataSet(paths, selections);
    auto output = RunService(step, input, vm);
    //output.PrintSummary(std::cout);

    if (output.GetNumberOfPartitions() > 0)
      writer.Write(output, "SST"); //outputEngineType);
  }
//  reader.Close();
  writer.Close();
}

static void
RunSSTBP(const boost::program_options::variables_map& vm)
{
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();
  std::string inputEngineType = "SST";
  std::string outputEngineType = "BPFile";

  std::cout<<"Run: "<<inputFname<<" "<<inputEngineType<<" --> "<<outputFname<<" "<<outputEngineType<<std::endl;

  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();
  int sleepTime = 0;
  if (!vm["sleep"].empty())
  {
    sleepTime = vm["sleep"].as<int>();
  }

  std::unique_ptr<fides::io::DataSetReader> FidesReader;
  if (jsonFile.empty())
  {
      FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(inputFname, fides::io::DataSetReader::DataModelInput::BPFile));
  }
  else
  {
    if (inputEngineType == "BPFile")
      FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(jsonFile));
    else
      FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(jsonFile, fides::io::DataSetReader::DataModelInput::JSONFile, true));
  }

  //fides::io::DataSetReader reader(jsonFile);
  fides::io::DataSetAppendWriter writer(outputFname);

  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  fides::metadata::MetaData selections;

  params["engine_type"] = inputEngineType;
  FidesReader->SetDataSourceParameters("source", params);

  std::cout<<"JSONFile: "<<jsonFile<<std::endl;

  int step = 0;
  while (true)
  {
    std::cout<<"Step: "<<step<<std::endl;
    if (sleepTime > 0)
      sleep(sleepTime);

    auto status = FidesReader->PrepareNextStep(paths);
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

    auto input = FidesReader->ReadDataSet(paths, selections);
    //input.PrintSummary(std::cout);

    auto output = RunService(step, input, vm);

    if (output.GetNumberOfPartitions() > 0)
      writer.Write(output, outputEngineType);
    step++;
  }
}

static void
RunSSTSST(const boost::program_options::variables_map& vm)
{
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();
  std::string inputEngineType = "SST";
  std::string outputEngineType = "SST";

  std::cout<<"Run: "<<inputFname<<" "<<inputEngineType<<" --> "<<outputFname<<" "<<outputEngineType<<std::endl;

  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();
  int sleepTime = 0;
  if (!vm["sleep"].empty())
  {
    sleepTime = vm["sleep"].as<int>();
  }

  fides::io::DataSetReader reader(jsonFile);
  fides::io::DataSetAppendWriter writer(outputFname);

  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  fides::metadata::MetaData selections;

  params["engine_type"] = inputEngineType;
  reader.SetDataSourceParameters("source", params);

  int step = 0;
  while (true)
  {
    std::cout<<"Step: "<<step<<std::endl;
    if (sleepTime > 0)
      sleep(sleepTime);

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

    auto input = reader.ReadDataSet(paths, selections);
    //input.PrintSummary(std::cout);

    auto output = RunService(step, input, vm);
    if (output.GetNumberOfPartitions() > 0)
      writer.Write(output, outputEngineType);
    step++;
  }
}

static void
RunIT(const boost::program_options::variables_map& vm)
{
  std::string inputEngineType = "BPFile", outputEngineType = "BPFile";
  if (!vm["input_engine"].empty())
    inputEngineType = vm["input_engine"].as<std::string>();
  if (!vm["output_engine"].empty())
    outputEngineType = vm["output_engine"].as<std::string>();

  if (inputEngineType == "BPFile")
  {
    if (outputEngineType == "BPFile")
      RunBPBP(vm);
    else if (outputEngineType == "SST")
      RunBPSST(vm);
    else
    {
      std::cout<<"ERROR: unknown output engine: "<<outputEngineType<<std::endl;
      return;
    }
  }
  else if (inputEngineType == "SST")
  {
    if (outputEngineType == "BPFile")
      RunSSTBP(vm);
    else if (outputEngineType == "SST")
      RunSSTSST(vm);
    else
    {
      std::cout<<"ERROR: unknown output engine: "<<outputEngineType<<std::endl;
      return;
    }
  }
  else
  {
    std::cout<<"ERROR: unknown input engine: "<<outputEngineType<<std::endl;
    return;
  }
  //RunBPSST(vm);
  //RunSSTBP(vm);
  return;

#if 0
  std::string inputFname = vm["file"].as<std::string>();
  std::string outputFname = vm["output"].as<std::string>();

  std::string inputEngineType = "BPFile";
  std::string outputEngineType = "BPFile";
  if (!vm["input_engine"].empty())
    inputEngineType = vm["input_engine"].as<std::string>();
  if (!vm["output_engine"].empty())
    outputEngineType = vm["output_engine"].as<std::string>();    
  
  std::cout<<"Run:: "<<inputFname<< " _"<<inputEngineType<<"_ --> ";
  std::cout<<outputFname<<" _"<<outputEngineType<<"_"<<std::endl;
						      
  std::string jsonFile = "";
  if (!vm["json"].empty())
    jsonFile = vm["json"].as<std::string>();
  //  else
  //    throw std::runtime_error("Error. SST requires a json file.");

  fides::io::DataSetReader *reader = nullptr;
  if (!jsonFile.empty())
  {
    if (inputEngineType == "BPFile")
      reader = new fides::io::DataSetReader(jsonFile);
    else if (inputEngineType == "SST")
      reader = new fides::io::DataSetReader(jsonFile); //, fides::io::DataSetReader::DataModelInput::JSONFile, true);
  }
  else
  {
    reader = new fides::io::DataSetReader(inputFname, fides::io::DataSetReader::DataModelInput::BPFile);
  }
    
  
  std::unordered_map<std::string, std::string> paths;
  paths["source"] = inputFname;

  fides::DataSourceParams params;
  fides::metadata::MetaData selections;

  params["engine_type"] = inputEngineType;
  reader->SetDataSourceParameters("source", params);

  fides::io::DataSetAppendWriter writer(outputFname);
  int totalNumSteps = -1;

  bool isBPFile = true;
  if (inputEngineType == "BPFile")
  {
    auto metaData = reader->ReadMetaData(paths);
    isBPFile = true;
    if (metaData.Has(fides::keys::NUMBER_OF_STEPS()))
      totalNumSteps = metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems;
  }
  else
    isBPFile = false;

  int step = 0;
  while (true)
  {
    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;
    if (isBPFile && step == totalNumSteps)
      break;

    //std::cout<<"step= "<<step<<std::endl;
    if (isBPFile)
    {
      selections.Set(fides::keys::STEP_SELECTION(), fides::metadata::Index(step));
    }
    else
    {
      if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;
      auto status = reader->PrepareNextStep(paths);
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
      if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;
    }

    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;

    vtkm::cont::PartitionedDataSet input;

    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;
    if (inputEngineType == "BPFile")
    {
      input = reader->ReadDataSet(paths, selections);
    }
    else
    {
      if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;          
      input = reader->ReadDataSet(paths, selections);
      if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;          
    }
      

    
    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;              
    //std::cout<<"NUMDS: "<<input.GetNumberOfPartitions()<<std::endl;
    //auto ds = input.GetPartition(0);
    //std::cout<<" numCS: "<<ds.GetNumberOfCoordinateSystems()<<std::endl;
    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;              
    auto output = RunService(input, vm);
    if (inputEngineType == "SST") std::cout<<"SST "<<__LINE__<<std::endl;              
    writer.Write(output, outputEngineType); //"BPFile");

    //fides::metadata::MetaData selections;
    //selections.Set(fides::keys::BLOCK_SELECTION(), blockSelection);

    //auto output = reader.ReadDataSet(paths, selections);
    //std::cout << "Step: " << step << " #ds= " << output.GetNumberOfPartitions() << std::endl;
    //RunService(reader, paths, writer, vm);

    step++;
  }
#endif
}

static void
RunSST2(const boost::program_options::variables_map& vm)
{
  std::cout<<"RunSST"<<std::endl;
  xenia::utils::DataSetWriter writer(vm);
  xenia::utils::DataSetReader reader(vm);

#if 0 //can we take this out??
//old code.
if (0)
{
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
  }
  #endif

  reader.Init();
  vtkm::Id step = 0;
  while (true)
  {
    auto status = reader.BeginStep();

    if (status == fides::StepStatus::NotReady)
      continue;
    else if (status == fides::StepStatus::EndOfStream)
      break;
    else if (status == fides::StepStatus::OK)
    {
      std::cout<<"SST read step= "<<step<<std::endl;
      auto output = reader.Read();
      std::cout<<"    np= "<<output.GetNumberOfPartitions()<<std::endl;
      step++;
    }
    else
      throw std::runtime_error("Unexpected status.");
  }

  /*

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
    std::cout << "Step: " << step << " #ds= " << output.GetNumberOfPartitions() << std::endl;
    RunService(writer, output, vm);

    step++;
  }
  */
}

int main(int argc, char** argv)
{
#ifdef ENABLE_MPI
  MPI_Init(NULL, NULL);
#endif

  //InitDebug();

  namespace po = boost::program_options;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("file", po::value<std::string>(), "Input file")
    ("json", po::value<std::string>(), "Fides JSON data model file")
    ("sleep", po::value<int>(), "Seconds to sleep between reads")
    ("remove-ghost-cells", po::value<std::string>(), "Remove ghost cells from dataset (specify the field name)")
    ("output", po::value<std::string>(), "Output file")
    ("input_engine", po::value<std::string>(), "Adios2 input engine type (BP, SST, or VTK)")
    ("output_engine", po::value<std::string>(), "Adios2 output engine type (BP or SST)")
    ("service", po::value<std::string>(), "Type of service to run (copier, streamline, contour, render)")
    ;

    //converter
    desc.add_options() ("vtkfile", po::value<std::string>(), "VTK output file");

    //contour
    desc.add_options()
      ("cell_to_point", "Average cell field to point")
      ("field", po::value<std::string>(), "field name in input data")
      ("isovals", po::value<std::vector<vtkm::FloatDefault>>(), "Isosurface values")
      ;

    //streamline
    desc.add_options()
    ("fieldx", po::value<std::string>(), "Name of x component of vector field in input data.")
    ("fieldy", po::value<std::string>(), "Name of x component of vector field in input data.")
    ("fieldz", po::value<std::string>(), "Name of x component of vector field in input data.")
    ("seed-point,s", po::value<std::vector<std::string>>(), "Seed point location. Separate components with spaces or commas. Can be specified multiple times for multiple seeds.")
    ("seed-grid-bounds", po::value<std::string>(), "Specify a the bounds for a grid of seed points. The values are specified as `minx maxx miny maxy minz maxz`.")
    ("seed-grid-dims", po::value<std::string>(), "Specify the number of seed points in each dimension of the seed grid. The values are specified as `numx numy numz`.")
    ("step-size", po::value<vtkm::FloatDefault>(), "Step size for particle advection.")
    ("max-steps", po::value<vtkm::Id>(), "Maximum number of steps.")
    ("tube-size", po::value<vtkm::FloatDefault>(), "If specified, create tube geometry with the given radius.")
    ("tube-num-sides", po::value<vtkm::IdComponent>(), "Number of sides around tubes (if generated).");

    std::cout<<__LINE__<<std::endl;
    //render
    desc.add_options()
    ("position", po::value<std::vector<float>>()->multitoken(), "Camera position")
    ("lookat", po::value<std::vector<float>>()->multitoken(), "Camera look at position")
    ("up", po::value<std::vector<float>>()->multitoken(), "Camera up direction")
    ("fov", po::value<float>(), "Camera up direction")
    ("clip", po::value<std::vector<float>>()->multitoken(), "Clipping range")
    ("imagesize", po::value<std::vector<int>>()->multitoken(), "Image size")
    ("scalar_range", po::value<std::vector<float>>()->multitoken(), "Scalar rendering range");
    std::cout<<__LINE__<<std::endl;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  std::cout<<__LINE__<<std::endl;

  if (vm.count("help"))
  {
    std::cout << desc << "\n";
    return 1;
  }

  if (vm["service"].empty())
  {
    std::cout<<"Error. No service specified"<<std::endl;
    std::cout<<desc<<std::endl;
    return 1;
  }

  //po::store(po::parse_command_line(argc, argv, desc), vm);
  //po::notify(vm);

  RunIT(vm);


  /*
  std::string engineType = "BP";
  if (!vm["input_engine"].empty())
    engineType = vm["input_engine"].as<std::string>();

  if (engineType == "SST")
    RunSST(vm);
  else if (engineType == "BP")
    RunBP(vm);
  else if (engineType == "VTK")
    RunVTK(vm);
  else
  {
    std::cout << "Unknown engine type: `" << engineType << "`\n";
    return 1;
  }
  */

#ifdef ENABLE_MPI
  MPI_Finalize();
#endif
  return 0;
}
