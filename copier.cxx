#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>
#include <adios2.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include <map>
#include <typeindex>
#include <numeric>

#include <vtkm/cont/ArrayHandleRunTimeVec.h>
#include <vtkm/cont/UnknownArrayHandle.h>

template <typename T>
static inline void
ReadData(const std::string& varName, adios2::IO& io, adios2::Engine& reader, vtkm::cont::UnknownArrayHandle& arr, adios2::Dims& shape)
{
  auto var = io.InquireVariable<T>(varName);
  shape = var.Shape();
  std::size_t dataSz = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<std::size_t>());
  std::vector<T> res(dataSz);

  reader.Get<T>(var, res.data(), adios2::Mode::Sync);
  auto dataAH = vtkm::cont::make_ArrayHandle(res, vtkm::CopyFlag::On);
  arr = vtkm::cont::UnknownArrayHandle(dataAH);
}

template <typename T>
static inline void
WriteData(const std::string& varName, adios2::IO& io, adios2::Engine& writer, const vtkm::cont::UnknownArrayHandle& arr, const adios2::Dims& varShape)
{
  std::vector<std::size_t> shape, start, count;
  for (std::size_t i = 0; i < varShape.size(); i++)
  {
    shape.push_back(varShape[i]);
    count.push_back(varShape[i]);
    start.push_back(0);
  }

  adios2::Variable<T> vOut = io.InquireVariable<T>(varName);
  if (!vOut)
    vOut = io.DefineVariable<T>(varName, shape, start, count);

  if (!arr.CanConvert<vtkm::cont::ArrayHandleBasic<T>>())
    throw std::string("Can convert type for " + varName);

  auto dataArray = arr.AsArrayHandle<vtkm::cont::ArrayHandleBasic<T>>();
  writer.Put(vOut, dataArray.GetReadPointer());
}

#define xeniaTemplateMacro(TYPE, call)               \
  {                                                  \
  switch(TYPE[0])                                    \
  {                                                  \
  case 'i' : {using VAR_TYPE=int; call; break;}      \
  case 'f' : {using VAR_TYPE=float; call; break;}    \
  case 'd' : {using VAR_TYPE=double; call; break; }  \
  default  : {throw std::string("Unsupported variable type: "+TYPE); break;} \
  }                                                                    \
  }                                                                    \

int main(int argc, char **argv)
{
  int rank = 0, numProcs = 1;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

  if (argc != 3)
  {
    std::cerr<<"****************************************************"<<std::endl;
    std::cerr<<"Error: Usage: "<<argv[0]<<" input output"<<std::endl;
    std::cerr<<"  Please rerun with the proper arguments."<<std::endl;
    std::cerr<<"  Thanks for stopping by!"<<std::endl<<std::endl;
    MPI_Finalize();
    return 0;
  }
  if (numProcs != 1)
  {
    std::cerr<<"****************************************************"<<std::endl;
    std::cerr<<" Error. This probably only works with 1 rank..."<<std::endl<<std::endl;
    MPI_Finalize();
    return 0;
  }

  adios2::ADIOS adios(MPI_COMM_WORLD);

  std::string inputFname(argv[1]), outputFname(argv[2]);

  if (rank == 0)
    std::cerr<<"Reading from: "<<inputFname<<std::endl;

  auto inIO = adios.DeclareIO("Input");
  auto reader = inIO.Open(inputFname, adios2::Mode::Read);

  auto outIO = adios.DeclareIO("Output");
  auto writer = outIO.Open(outputFname, adios2::Mode::Write);

  int step = 0;
  while (true)
  {
    auto status = reader.BeginStep();
    if (status != adios2::StepStatus::OK)
      break;

    auto variables = inIO.AvailableVariables();
    std::cout<<"Reading Step= "<<step<<std::endl;
    for (const auto &vi : variables)
    {
      vtkm::cont::UnknownArrayHandle arr;
      adios2::Dims shape;
      auto varType = inIO.VariableType(vi.first);
      xeniaTemplateMacro(varType, ReadData<VAR_TYPE>(vi.first, inIO, reader, arr, shape));
      xeniaTemplateMacro(varType, WriteData<VAR_TYPE>(vi.first, outIO, writer, arr, shape));
    }

    if (step == 0)
    {
      auto allAttrs = inIO.AvailableAttributes();
      for (const auto &ai : allAttrs)
      {
        const auto a = inIO.InquireAttribute<std::string>(ai.first);
        outIO.DefineAttribute<std::string>(a.Name(), a.Data()[0]);
      }
    }
    reader.EndStep();
    writer.EndStep();
    step++;
  }

  reader.Close();
  writer.Close();
  MPI_Finalize();

  return 0;
}
