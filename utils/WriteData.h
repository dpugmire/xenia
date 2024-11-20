#pragma once

#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/io/VTKDataSetWriter.h>
#include <boost/program_options.hpp>

#include <fides/DataSetWriter.h>
#include <string>
#include <regex>

#ifdef ENABLE_MPI
#include <mpi.h>
#endif

namespace xenia
{
namespace utils
{

inline void
WriteData(const vtkm::cont::PartitionedDataSet& pds, const std::string& outName)
{
  if (outName.find(".vtk") != std::string::npos)
  {
    if (pds.GetNumberOfPartitions() == 1)
    {
      vtkm::io::VTKDataSetWriter writer(outName);
      writer.SetFileTypeToBinary();
      writer.WriteDataSet(pds.GetPartition(0));
    }
    else
    {
      for (vtkm::Id i = 0; i < pds.GetNumberOfPartitions(); i++)
      {
        std::string fname = outName;
        std::string extension = "_" + std::to_string(i) + ".vtk";
        std::regex_replace(fname, std::regex(".vtk"), extension);
        vtkm::io::VTKDataSetWriter writer(fname);
        writer.SetFileTypeToBinary();
        writer.WriteDataSet(pds.GetPartition(i));
      }
    }
  }
  else if (outName.find(".bp") != std::string::npos)
  {
    fides::io::DataSetWriter writer(outName);
    writer.Write(pds, "BPFile");
  }
  else
  {
    std::cerr<<" Error. Unsupported extension: "<<outName<<std::endl;
  }
}

class DataSetWriter
{
  public:
  DataSetWriter(const boost::program_options::variables_map& vm);

  bool WriteDataSet(const vtkm::cont::PartitionedDataSet& pds);

  void BeginStep() {}
  void EndStep() {this->Step++;}
  void Close();

  void SetTimeVaryingOutput(bool val) { this->TimeVaryingOutput = val; }
  bool GetTimeVaryingOutput() const { return this->TimeVaryingOutput; }

  enum OutputFileType
  {
    NONE = 0,
    VTK = 1,
    BP = 2,
    SST = 3,
  };

  bool WriteVTK(const vtkm::cont::PartitionedDataSet& pds);
  bool WriteBP(const vtkm::cont::PartitionedDataSet& pds);

  private:
  void CreateVisItFile(int totalNumDS);
  void AppendVTKFiles(int totalNumDS) const;
  std::vector<std::string> GetVTKOutputFileNames(int totalNumDS, int blk0, int blk1) const;
  std::string VisItFileName;

  OutputFileType OutputType = OutputFileType::NONE;
  std::string OutputFileName;
  std::unique_ptr<fides::io::DataSetAppendWriter> Writer;
  vtkm::Id Step = 0;
  bool TimeVaryingOutput = false;

  int Rank = 0;
  int NumRanks = 1;
};


}
} //xenia::utils
