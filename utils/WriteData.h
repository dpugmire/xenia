#pragma once

#include "CommandLineArgParser.h"
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <fides/DataSetWriter.h>
#include <string>
#include <regex>

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

}
} //xenia::utils
