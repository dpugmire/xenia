#include <stdio.h>
#include "WriteData.h"

#include <vtkm/io/VTKDataSetWriter.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <numeric>

namespace xenia
{
namespace utils
{

DataSetWriter::DataSetWriter(const boost::program_options::variables_map& vm)
: Writer(nullptr)
{
#ifdef ENABLE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &this->Rank);
  MPI_Comm_size(MPI_COMM_WORLD, &this->NumRanks);
#endif

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

void DataSetWriter::CreateVisItFile(int totalNumDS) const
{
    if (this->Rank != 0)
        return;

    auto pos = this->OutputFileName.find(".vtk");
    std::string visitFile = this->OutputFileName;
    std::string pattern(".%d.visit");
    visitFile.replace(pos, pattern.size(), pattern);

    int timestep = static_cast<int>(this->Step);
    char buffer[128];
    snprintf(buffer, sizeof(buffer), visitFile.c_str(), timestep);

    std::cout<<"visitFile: "<<buffer<<std::endl;
    std::ofstream fout(buffer, std::ofstream::out);

    auto tmp = this->OutputFileName;
    pattern = ".ts_%d_ds_%d.vtk";
    tmp.replace(pos, pattern.size(), pattern);
    fout<<"!NBLOCKS "<<totalNumDS<<std::endl;
    for (int i = 0; i < totalNumDS; i++)
    {
        snprintf(buffer, sizeof(buffer), tmp.c_str(), timestep, i);
        fout<<buffer<<std::endl;
        std::cout<<"  "<<buffer<<std::endl;
    }
}

std::vector<std::string> DataSetWriter::GetVTKOutputFileNames(int totalNumDS, int blk0, int blk1) const
{
    std::vector<std::string> outputFileNames;

    std::string fname;
    int timestep = static_cast<int>(this->Step);

    auto outputFileName = this->OutputFileName;

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


bool DataSetWriter::WriteVTK(const vtkm::cont::PartitionedDataSet& pds)
{
    int localNumDS = static_cast<int>(pds.GetNumberOfPartitions());
    int totalNumDS = localNumDS;
    int b0 = 0, b1 = localNumDS;
#ifdef ENABLE_MPI
    std::vector<int> allNumDS;
    allNumDS.resize(this->NumRanks, 0);
    allNumDS[this->Rank] = localNumDS;
    MPI_Allreduce(MPI_IN_PLACE, allNumDS.data(), this->NumRanks, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    totalNumDS = std::accumulate(allNumDS.begin(), allNumDS.end(), 0);

    //set block index bounds for this rank.
    for (int i = 0; i < this->Rank; i++)
        b0 += allNumDS[i];
    b1 = b0 + localNumDS;
    //std::cout<<this->Rank<<": "<<localNumDS<<" "<<totalNumDS<<" ("<<b0<<" "<<b1<<")"<<std::endl;
#endif

    if (totalNumDS == 0)
        return false;

    this->CreateVisItFile(totalNumDS);
    auto outputFileNames = this->GetVTKOutputFileNames(totalNumDS, b0, b1);

    int blkIdx = 0;
    for (const auto& ds : pds.GetPartitions())
    {
        vtkm::io::VTKDataSetWriter writer(outputFileNames[blkIdx]);
        writer.WriteDataSet(ds);
        blkIdx++;
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