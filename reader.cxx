#include <fides/DataSetReader.h>
#ifdef FIDES_USE_MPI
#include <mpi.h>
#endif
#include <string>
#include <unordered_map>
#include <vector>

//#include <vtkm/cont/Algorithm.h>
//#include <vtkm/cont/ArrayRangeCompute.h>
//#include <vtkm/worklet/DispatcherMapTopology.h>
//#include <vtkm/worklet/ScatterPermutation.h>
//#include <vtkm/worklet/WorkletMapTopology.h>


int main(int argc, char** argv)
{
  fides::io::DataSetReader reader(argv[1]);



  return 0;
}
