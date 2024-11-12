#include "ReadData.h"
#include "CommandLineArgParser.h"

#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/io/VTKDataSetReader.h>

#include <fides/DataSetReader.h>

namespace xenia
{
namespace utils
{

const std::set<std::string> DataSetReader::ValidEngineTypes({"BP5", "SST"});

DataSetReader::DataSetReader(const boost::program_options::variables_map& vm)
{
  std::cout<<"building dataset reader."<<std::endl;
  if (!vm["file"].empty())
  {
    this->FileName = vm["file"].as<std::string>();
    if (this->FileName.find(".bp") == std::string::npos)
      throw std::runtime_error("Error. Only BP files supported.");
    this->Paths["source"] = this->FileName;
  }
  else if (vm["json"].empty())
  {
    throw std::runtime_error("Error. Must specify --file or --json (or both).");
  }

  if (!vm["input_engine"].empty())
    this->EngineType = vm["input_engine"].as<std::string>();
  if (this->ValidEngineTypes.find(this->EngineType) == this->ValidEngineTypes.end())
    throw std::runtime_error("Invalid engine type: " + this->EngineType);

  this->Paths["engine_type"] = this->EngineType;

  if (vm.count("json") == 1)
  {
    this->JSONFile = vm["json"].as<std::string>();
    if (this->EngineType == "BP5")
      this->FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(this->JSONFile));
    else if (this->EngineType == "SST")
      this->FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(this->JSONFile, fides::io::DataSetReader::DataModelInput::JSONFile, true));
  }
  else
  {
    this->FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(this->FileName, fides::io::DataSetReader::DataModelInput::BPFile));
    //this->FidesReader = std::unique_ptr<fides::io::DataSetReader>(new fides::io::DataSetReader(this->FileName));
  }

#ifdef ENABLE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &this->Rank);
  MPI_Comm_size(MPI_COMM_WORLD, &this->NumRanks);
#endif

  //Get number of blocks from the Source.
  /*
  auto& nBlocks = this->MetaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_BLOCKS());
  const auto& sourceNames = this->FidesReader->GetDataSourceNames();
  if (sourceNames.empty())
    throw std::string("Error. No data sources");
  */

#if 0
  this->BlockSelection.clear();
  if (rank == 0)
  {
    this->BlockSelection.push_back(0);
    this->BlockSelection.push_back(1);
    this->BlockSelection.push_back(2);
  }
  else
  {
    //this->BlockSelection.push_back(3);
    //this->BlockSelection.push_back(4);
    //this->BlockSelection.push_back(5);
    //this->BlockSelection.push_back(6);
    this->BlockSelection.push_back(7);
  }
  #endif
}

void
DataSetReader::InitBlockSelection()
{
    this->BlockSelection.clear();

    int nBlocks = static_cast<int>(this->MetaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_BLOCKS()).NumberOfItems);
    std::cout<<"nBlocks = "<<nBlocks<<std::endl;

    // Default is to load everything.
    if (this->NumRanks == 1)
      return;

    int nPer = nBlocks / this->NumRanks;
    int remainder = nBlocks % this->NumRanks;

    int b0 = this->Rank * nPer;
    int b1 = b0 + nPer;
    if (this->Rank == this->NumRanks-1)
      b1 = nBlocks;

    for (int b = b0; b < b1; b++)
      this->BlockSelection.push_back(b);

    std::cout<<"Rank: "<<this->Rank<<" has blocks: "<<b0<<" "<<b1<<std::endl;
}

void
DataSetReader::Init()
{
  this->MetaData = this->FidesReader->ReadMetaData(this->Paths);
  this->InitCalled = true;

  if (this->MetaData.Has(fides::keys::NUMBER_OF_BLOCKS()))
    this->InitBlockSelection();

  //For BPfile method...
  if (this->EngineType == "BP5")
  {
    if (this->MetaData.Has(fides::keys::NUMBER_OF_STEPS()))
      this->NumSteps = this->MetaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems;
    else
      this->NumSteps = 1;
    std::cout<<"***** NSTEPS= "<<this->NumSteps<<std::endl;
  }
  else if (this->EngineType == "SST")
  {
    std::cout<<"SST init."<<std::endl;
  }

}

void
DataSetReader::SetBlocksMetaData(fides::metadata::MetaData& md) const
{
  int rank = 0, numProcs = 1;
#ifdef ENABLE_MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
#endif

  //Only needed for more than 1 rank.
  if (numProcs == 1)
    return;

  std::size_t numBlocks = 0;
  if (this->MetaData.Has(fides::keys::NUMBER_OF_BLOCKS()))
    numBlocks = this->MetaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_BLOCKS()).NumberOfItems;

  if (numBlocks == 0)
    return;
}

vtkm::cont::PartitionedDataSet DataSetReader::Read()
{
  auto md = this->MetaData;

  if (this->EngineType == "BP5")
    md.Set(fides::keys::STEP_SELECTION(), fides::metadata::Index(this->Step));

  this->SetBlocksMetaData(md);

  if (!this->BlockSelection.empty())
  {
    fides::metadata::Vector<std::size_t> blockSel(this->BlockSelection);
    md.Set(fides::keys::BLOCK_SELECTION(), blockSel);
  }

  return this->FidesReader->ReadDataSet(this->Paths, md);
}

}
} //xenia::utils
