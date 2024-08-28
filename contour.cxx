#include <mpi.h>
#include <vector>
#include <boost/program_options.hpp>

#include "utils/ReadData.h"
#include "utils/WriteData.h"

#include <fides/DataSetReader.h>
#include <fides/DataSetWriter.h>

#include <vtkm/filter/contour/Contour.h>

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
    ("isovals", po::value<std::vector<vtkm::FloatDefault>>(), "Isosurface values")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

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
}
