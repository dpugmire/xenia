#include <ios>       //std::ios_base::failure
#include <iostream>  //std::cout
#include <stdexcept> //std::invalid_argument std::exception
#include <vector>
#include <adios2.h>
#include <stdio.h>
#include <math.h>
#if ADIOS2_USE_MPI
#include <mpi.h>
#endif
#include <mpi.h>
#define PI 3.141592653589793
using namespace std;

#include <vtkm/Types.h>

// I need two l
// one for sending the plane
// one for going through the cube
// rember k is for thickness
// k changes when sending the sheets beacuse we forget about a dention
// 7 point stencil

void gencore(int rank, vtkm::FloatDefault h, std::vector<vtkm::FloatDefault> &c)
{
  for (std::size_t i = 0; i < c.size(); i++)
    c[i] = vtkm::FloatDefault(i)*h;
}

// I added r norm
void evalF(int rank, vtkm::FloatDefault t, std::vector<vtkm::FloatDefault> &x, std::vector<vtkm::FloatDefault> &z, std::vector<vtkm::FloatDefault> &y, std::vector<vtkm::FloatDefault> &F)
{
    // vtkm::FloatDefault r_norm3 = pow(r_norm, 3);
    // vtkm::FloatDefault r_norm5 = pow(r_norm, 5);

  for (int j = 0; j < y.size(); j++) //  j= row
  {
    for (int k = 0; k < z.size(); k++)
    {
      for (int i = 0; i < x.size(); i++) // i =col
      {
        int L = i + x.size() * k + j * x.size() * z.size();
        F[L] = pow(x[i], 4) * exp(-y[j] * z[k]);
      }
    }
  }
}

void calcLapace(int rank, int size, int nx, int nz, int ny, vtkm::FloatDefault h, std::vector<vtkm::FloatDefault> &F, std::vector<vtkm::FloatDefault> &laplace)
{
    MPI_Status status;
    MPI_Request req_send_forward;
    MPI_Request req_send_backward;
    std::vector<vtkm::FloatDefault> forward_neighbor(nx * nz);
    std::vector<vtkm::FloatDefault> backward_neighbor(nx * nz);
    forward_neighbor.resize(nx * nz, 0);
    backward_neighbor.resize(nx * nz, 0);
    // ny = rows
    // nx = cols
    // nz = thickness
    for (int k = 0; k < nz; k++)
    {
        for (int i = 0; i < nx; i++)
        {
            int j = ny - 1;
            int L = i + nx * k + j * nx * nz; // global movemnent in cube
            int l = i + nx * k;               // local movement in plane
            forward_neighbor[l] = F[L];
        }
    }
    for (int k = 0; k < nz; k++)
    {
        for (int i = 0; i < nx; i++)
        {
            int j = 0;
            int L = i + nx * k + j * nx * nz; // global movemnent in cube
            int l = i + nx * k;               // local movement in plane
            backward_neighbor[l] = F[L];
        }
    }

    if (rank == 0)
    {

        MPI_Isend(forward_neighbor.data(), nx, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD, &req_send_forward);
        MPI_Recv(forward_neighbor.data(), nx, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD, &status);
    }
    else if (rank == size - 1)
    {
        MPI_Isend(backward_neighbor.data(), nx, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, &req_send_backward);
        MPI_Recv(backward_neighbor.data(), nx, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, &status);
    }
    else
    {
        MPI_Isend(backward_neighbor.data(), nx, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, &req_send_backward);
        MPI_Isend(forward_neighbor.data(), nx, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD, &req_send_forward);
        MPI_Recv(backward_neighbor.data(), nx, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(forward_neighbor.data(), nx, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD, &status);
    }

    //  in the stencil -6F[L]
    // start

    if (rank == 0)
    {

        // normal vals
        // can't go in here the loops
        for (int j = 1; j < ny - 1; j++) // ny = row index =j
        {

            for (int k = 1; k < nz - 1; k++) // nz = thickness =k
            {
                for (int i = 1; i < nx - 1; i++) // nx = column index=i
                {
                    int L = i + nx * k + j * nx * nz;          // cols *row_index + col = location
                    int ljp1 = i + nx * k + (j + 1) * nx * nz; // row + 1
                    int ljm1 = i + nx * k + (j - 1) * nx * nz; // row - 1
                    int lip1 = (i + 1) + nx * k + j * nx * nz; // column + 1
                    int lim1 = (i - 1) + nx * k + j * nx * nz; // column - 1
                    int lkp1 = i + nx * (k + 1) + j * nx * nz;
                    int lkm1 = i + nx * (k - 1) + j * nx * nz;
                    laplace[L] = (F[lip1] + F[lim1] + F[ljp1] + F[ljm1] + F[lkp1] + F[lkm1] - 6 * F[L]) / (h * h);
                }
            }
        }
        // special case 0 proc
        // last row therefore
        // row constant ny-1
        for (int k = 1; k < nz - 1; k++)
        {
            for (int i = 1; i < nx - 1; i++)
            {
                int j = ny - 1; // row index
                int l = i + nx * k;
                int L = i + nx * k + j * nx * nz;          // cols *row_index + col = location
                int ljp1 = i + nx * k + (j + 1) * nx * nz; // row + 1
                int ljm1 = i + nx * k + (j - 1) * nx * nz; // row - 1
                int lip1 = (i + 1) + nx * k + j * nx * nz; // column + 1
                int lim1 = (i - 1) + nx * k + j * nx * nz; // column - 1
                int lkp1 = i + nx * (k + 1) + j * nx * nz;
                int lkm1 = i + nx * (k - 1) + j * nx * nz;

                laplace[L] = (F[lip1] + F[lim1] + forward_neighbor[l] + F[ljm1] + F[lkp1] + F[lkm1] - 6 * F[L]) / (h * h);
            }
        }
    }
    // last proc

    else if (rank == size - 1)
    {
        // normal vals
        for (int j = 1; j < ny - 1; j++) // ny = row index =j
        {

            for (int k = 1; k < nz - 1; k++) // nz = thickness =k
            {
                for (int i = 1; i < nx - 1; i++) // nx = column index=i
                {
                    int L = i + nx * k + j * nx * nz;          // cols *row_index + col = location
                    int ljp1 = i + nx * k + (j + 1) * nx * nz; // row + 1
                    int ljm1 = i + nx * k + (j - 1) * nx * nz; // row - 1
                    int lip1 = (i + 1) + nx * k + j * nx * nz; // column + 1
                    int lim1 = (i - 1) + nx * k + j * nx * nz; // column - 1
                    int lkp1 = i + nx * (k + 1) + j * nx * nz;
                    int lkm1 = i + nx * (k - 1) + j * nx * nz;
                    laplace[L] = (F[lip1] + F[lim1] + F[ljp1] + F[ljm1] + F[lkp1] + F[lkm1] - 6 * F[L]) / (h * h);
                }
            }
        }
        // special case size-1 proc
        // last row therefore
        // row constant 0
        for (int k = 1; k < nz - 1; k++)
        {
            for (int i = 1; i < nx - 1; i++)
            {
                int j = 0; // row index
                int l = i + nx * k;
                int L = i + nx * k + j * nx * nz;          // cols *row_index + col = location
                int ljp1 = i + nx * k + (j + 1) * nx * nz; // row + 1
                int ljm1 = i + nx * k + (j - 1) * nx * nz; // row - 1
                int lip1 = (i + 1) + nx * k + j * nx * nz; // column + 1
                int lim1 = (i - 1) + nx * k + j * nx * nz; // column - 1
                int lkp1 = i + nx * (k + 1) + j * nx * nz;
                int lkm1 = i + nx * (k - 1) + j * nx * nz;

                laplace[L] = (F[lip1] + F[lim1] + F[ljp1] + backward_neighbor[l] + F[lkp1] + F[lkm1] - 6 * F[L]) / (h * h);
            }
        }
    }
    else
    {
        // normal vals
        for (int j = 1; j < ny - 1; j++) // ny = row index =j
        {

            for (int k = 1; k < nz - 1; k++) // nz = thickness =k
            {
                for (int i = 1; i < nx - 1; i++) // nx = column index=i
                {
                    int L = i + nx * k + j * nx * nz;          // cols *row_index + col = location
                    int ljp1 = i + nx * k + (j + 1) * nx * nz; // row + 1
                    int ljm1 = i + nx * k + (j - 1) * nx * nz; // row - 1
                    int lip1 = (i + 1) + nx * k + j * nx * nz; // column + 1
                    int lim1 = (i - 1) + nx * k + j * nx * nz; // column - 1
                    int lkp1 = i + nx * (k + 1) + j * nx * nz;
                    int lkm1 = i + nx * (k - 1) + j * nx * nz;
                    laplace[L] = (F[lip1] + F[lim1] + F[ljp1] + F[ljm1] + F[lkp1] + F[lkm1] - 6 * F[L]) / (h * h);
                }
            }
        }
        // special case size-1 proc
        // last row therefore
        // row constant last

        for (int k = 1; k < nz - 1; k++)
        {
            for (int i = 1; i < nx - 1; i++)
            {
                int j = 0; // row index
                int l = i + nx * k;
                int L = i + nx * k + j * nx * nz;          // cols *row_index + col = location
                int ljp1 = i + nx * k + (j + 1) * nx * nz; // row + 1
                int ljm1 = i + nx * k + (j - 1) * nx * nz; // row - 1
                int lip1 = (i + 1) + nx * k + j * nx * nz; // column + 1
                int lim1 = (i - 1) + nx * k + j * nx * nz; // column - 1
                int lkp1 = i + nx * (k + 1) + j * nx * nz;
                int lkm1 = i + nx * (k - 1) + j * nx * nz;

                laplace[L] = (F[lip1] + F[lim1] + F[ljp1] + backward_neighbor[l] + F[lkp1] + F[lkm1] - 6 * F[L]) / (h * h);
            }
        }

        for (int k = 1; k < nz - 1; k++)
        {
            for (int i = 1; i < nx - 1; i++)
            {
                int j = ny - 1; // row index
                int l = i + nx * k;
                int L = i + nx * k + j * nx * nz;          // cols *row_index + col = location
                int ljp1 = i + nx * k + (j + 1) * nx * nz; // row + 1
                int ljm1 = i + nx * k + (j - 1) * nx * nz; // row - 1
                int lip1 = (i + 1) + nx * k + j * nx * nz; // column + 1
                int lim1 = (i - 1) + nx * k + j * nx * nz; // column - 1
                int lkp1 = i + nx * (k + 1) + j * nx * nz;
                int lkm1 = i + nx * (k - 1) + j * nx * nz;

                laplace[L] = (F[lip1] + F[lim1] + forward_neighbor[l] + F[ljm1] + F[lkp1] + F[lkm1] - 6 * F[L]) / (h * h);
            }
        }
    }
}

int main(int argc, char *argv[])
{
  int rank = 0, size = 1;
#if ADIOS2_USE_MPI

  int provided;
  // MPI_THREAD_MULTIPLE is only required if you enable the SST MPI_DP

  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  // rank is giving a number to each proccess
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  std::cout << "The rank is  " << rank << endl;
  //  the size of something
  MPI_Comm_size(MPI_COMM_WORLD, &size);

#else
  MPI_Init(&argc, &argv);
  // rank is giving a number to each proccess
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //  the size of something
  MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif

#if ADIOS2_USE_MPI
  //adios2::ADIOS adios("adios2.xml", MPI_COMM_WORLD);
  adios2::ADIOS adios(MPI_COMM_WORLD);
#else
  adios2::ADIOS adios("adios2.xml");

#endif

  if (argc < 6)
  {
    cout << "Minimum 5 arguments required!!!!" << std::endl;
    return 1;
  }

  std::string filename = argv[1];
  // rows = y cols =x
  // e.bp 5y 4x 1
  // cols = constant
  // rows = leny*size
  // rows =5 (3)= 15
  size_t leny = atoi(argv[2]);
  size_t lenz = atoi(argv[3]);
  size_t lenx = atoi(argv[4]);
  int tmax = atoi(argv[5]);
  std::vector<vtkm::FloatDefault> x(lenx), z(leny), y(lenz);

  vtkm::FloatDefault h = 10.0 / (lenx - 1);
  // formula is why  L=i+nx*z+j*nx*nz
  std::vector<vtkm::FloatDefault> F(lenx*leny*lenz, 0.0);
  std::vector<vtkm::FloatDefault> laplace(lenx*leny*lenz, 0.0);

  vtkm::FloatDefault mx = 1.0, my = 0.0, mz = 0.0; // Dipole moment along the x-axis
  // F   15X4
  adios2::IO bpIO = adios.DeclareIO("BPFile_N2N");
  adios2::Engine bpWriter = bpIO.Open(filename, adios2::Mode::Write);
  adios2::Variable<vtkm::FloatDefault> xOut = bpIO.DefineVariable<vtkm::FloatDefault>(
    "x", {lenx}, {0}, {lenx}, adios2::ConstantDims);
  adios2::Variable<vtkm::FloatDefault> zOut = bpIO.DefineVariable<vtkm::FloatDefault>(
    "z", {lenz}, {0}, {lenz}, adios2::ConstantDims);
  adios2::Variable<vtkm::FloatDefault> yOut = bpIO.DefineVariable<vtkm::FloatDefault>(
    "y", {leny * size}, {rank * leny}, {leny}, adios2::ConstantDims);
  adios2::Variable<vtkm::FloatDefault> fOut = bpIO.DefineVariable<vtkm::FloatDefault>(
    "F", {size * leny, lenz, lenx}, {rank * leny, 0, 0}, {leny, lenz, lenx}, adios2::ConstantDims);
  // adios2::Variable<vtkm::FloatDefault> fOut = bpIO.DefineVariable<vtkm::FloatDefault>(
  //     "F", {leny * lenx}, {rank * lenx}, {lenx}, adios2::ConstantDims);
  //  maybe fix this
  adios2::Variable<vtkm::FloatDefault> lOut = bpIO.DefineVariable<vtkm::FloatDefault>(
    "Laplace", {size * leny, lenz, lenx}, {rank * leny, 0, 0}, {leny, lenz, lenx}, adios2::ConstantDims);
  // gencore fills in the values of the x, z, y array on each rank
  gencore(rank, h, x);
  gencore(rank, h, y);
  gencore(rank, h, z);

    const std::string extent = "0 " + std::to_string(size * leny - 1) + " 0 " + std::to_string(lenz - 1) + " 0 " + std::to_string(lenx - 1);

    const std::string imageData = R"(

<?xml version="1.0"?>

<VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">

<ImageData WholeExtent=")" + extent +
                                  R"(" Origin="0 0 0" Spacing="1 1 1">
<Piece Extent=")" + extent +
                                  R"(">
<PointData Scalars="F">

<DataArray Name="F" />

<DataArray Name="Laplace" />

<DataArray Name="TIME"> step
</DataArray>

</PointData>

</Piece>

</ImageData>

</VTKFile>)";


  bpIO.DefineAttribute<std::string>("vtk.xml", imageData);
  bpIO.DefineAttribute<std::string>("meow", "meow meow ");

  // evalF fills in the value of the two dimensional array F

  for (int t = 1; t <= tmax; t++)
  {
    vtkm::FloatDefault dt = vtkm::FloatDefault(t) / vtkm::FloatDefault(tmax);
    evalF(rank, t, x, z, y, F);

    if (size > 1)
    {
      calcLapace(rank, size, x.size(), z.size(), y.size(), h, F, laplace);
    }
    bpWriter.BeginStep();
    /** Put variables for buffering, template type is optional */
    bpWriter.Put(xOut, x.data());
    bpWriter.Put(zOut, z.data());
    bpWriter.Put(yOut, y.data());
    bpWriter.Put(fOut, F.data());
    bpWriter.Put(lOut, laplace.data());
    bpWriter.EndStep();
  }

  /** Create bp file, engine becomes unreachable after this*/

  bpWriter.Close();

#if ADIOS2_USE_MPI
  MPI_Finalize();
#endif

  return 0;
}
