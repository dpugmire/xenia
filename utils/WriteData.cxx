#include <stdio.h>
#include "WriteData.h"

#include <vtkm/io/VTKDataSetWriter.h>
#include <vtkm/cont/PartitionedDataSet.h>

namespace xenia
{
namespace utils
{

DataSetWriter::DataSetWriter(const boost::program_options::variables_map& vm)
: Writer(nullptr)
{
    this->OutputFileName = vm["output"].as<std::string>();
    if (this->OutputFileName.find(".vtk") != std::string::npos)
        this->OutputType = OutputFileType::VTK;
    else if (this->OutputFileName.find(".bp") != std::string::npos)
    {
        this->OutputType = OutputFileType::BP;
        if (!vm["output_engine"].empty())
        {
            auto engine = vm["output_engine"].as<std::string>();
            if (engine == "SST")
                this->OutputType = OutputFileType::SST;
            else if (engine == "BP")
                this->OutputType = OutputFileType::BP;
            else
                throw std::runtime_error("Error. Unknown engine type: " + engine);
        }

        this->Writer = std::unique_ptr<fides::io::DataSetAppendWriter>(new fides::io::DataSetAppendWriter(this->OutputFileName));
    }
}

bool DataSetWriter::WriteDataSet(const vtkm::cont::PartitionedDataSet& pds)
{
    if (this->OutputType == OutputFileType::VTK)
        return this->WriteVTK(pds);
    else if (this->Writer != nullptr)
    {
        if (this->OutputType == OutputFileType::BP)
            this->Writer->Write(pds, "BPFile");
        else if (this->OutputType == OutputFileType::SST)
            this->Writer->Write(pds, "SST");
        else
            return false;

        return true;
    }
    return false;
}

void
DataSetWriter::Close()
{
    if (this->Writer != nullptr)
        this->Writer->Close();
}

bool DataSetWriter::WriteVTK(const vtkm::cont::PartitionedDataSet& pds)
{
    auto numDS = pds.GetNumberOfPartitions();
    if (numDS == 0)
        return false;

    std::string outputFileName;
    //see if we need to format
    if (this->OutputFileName.find('%') != std::string::npos)
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), this->OutputFileName.c_str(), this->Step);
        outputFileName = buffer;
    }
    else
        outputFileName = this->OutputFileName;

    if (numDS == 1 )
    {
        vtkm::io::VTKDataSetWriter writer(outputFileName);
        writer.WriteDataSet(pds.GetPartition(0));
    }
    else
    {
        auto pos = outputFileName.find(".vtk");
        std::string pattern(".ds_%d.vtk");
        outputFileName.replace(pos, pattern.size(), pattern);
        for (vtkm::Id i = 0; i < numDS; i++)
        {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), outputFileName.c_str(), i);
            vtkm::io::VTKDataSetWriter writer(buffer);
            writer.WriteDataSet(pds.GetPartition(i));
        }
    }

    return true;
}

bool DataSetWriter::WriteBP(const vtkm::cont::PartitionedDataSet& pds)
{
    if (this->Writer == nullptr)
        return false;

    this->Writer->Write(pds, "BPFile");
    return true;
}
  
}
} //xenia::utils