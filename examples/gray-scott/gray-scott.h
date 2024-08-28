/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#ifndef __GRAY_SCOTT_H__
#define __GRAY_SCOTT_H__

#include <random>
#include <vector>

#include <mpi.h>

#include "settings.h"

class GrayScott
{
public:
    // Dimension of process grid
    size_t npx, npy, npz;
    // Coordinate of this rank in process grid
    size_t px, py, pz;
    // Dimension of local array
    size_t size_x, size_y, size_z;
    // Offset of local array in the global array
    size_t offset_x, offset_y, offset_z;

    GrayScott(const Settings &settings, MPI_Comm comm);
    ~GrayScott();

    void init();
    void iterate();
    void restart(std::vector<float> &u, std::vector<float> &v);

    const std::vector<float> &u_ghost() const;
    const std::vector<float> &v_ghost() const;

    std::vector<float> u_noghost() const;
    std::vector<float> v_noghost() const;

    void u_noghost(float *u_no_ghost) const;
    void v_noghost(float *v_no_ghost) const;

protected:
    Settings settings;

    std::vector<float> u, v, u2, v2;

    int rank, procs;
    int west, east, up, down, north, south;
    MPI_Comm comm;
    MPI_Comm cart_comm;

    // MPI datatypes for halo exchange
    MPI_Datatype xy_face_type;
    MPI_Datatype xz_face_type;
    MPI_Datatype yz_face_type;

    std::random_device rand_dev;
    std::mt19937 mt_gen;
    std::uniform_real_distribution<float> uniform_dist;

    // Setup cartesian communicator data types
    void init_mpi();
    // Setup initial conditions
    void init_field();

    // Progess simulation for one timestep
    void calc(const std::vector<float> &u, const std::vector<float> &v, std::vector<float> &u2,
              std::vector<float> &v2);
    // Compute reaction term for U
    float calcU(float tu, float tv) const;
    // Compute reaction term for V
    float calcV(float tu, float tv) const;
    // Compute laplacian of field s at (ix, iy, iz)
    float laplacian(int ix, int iy, int iz, const std::vector<float> &s) const;

    // Exchange faces with neighbors
    void exchange(std::vector<float> &u, std::vector<float> &v) const;
    // Exchange XY faces with north/south
    void exchange_xy(std::vector<float> &local_data) const;
    // Exchange XZ faces with up/down
    void exchange_xz(std::vector<float> &local_data) const;
    // Exchange YZ faces with west/east
    void exchange_yz(std::vector<float> &local_data) const;

    // Return a copy of data with ghosts removed
    std::vector<float> data_noghost(const std::vector<float> &data) const;

    // pointer version
    void data_noghost(const std::vector<float> &data, float *no_ghost) const;

    // Check if point is included in my subdomain
    inline bool is_inside(int x, int y, int z) const
    {
        if (x < static_cast<int>(offset_x))
            return false;
        if (x >= static_cast<int>(offset_x + size_x))
            return false;
        if (y < static_cast<int>(offset_y))
            return false;
        if (y >= static_cast<int>(offset_y + size_y))
            return false;
        if (z < static_cast<int>(offset_z))
            return false;
        if (z >= static_cast<int>(offset_z + size_z))
            return false;

        return true;
    }
    // Convert global coordinate to local index
    inline int g2i(int gx, int gy, int gz) const
    {
        const int x = static_cast<int>(gx - offset_x);
        const int y = static_cast<int>(gy - offset_y);
        const int z = static_cast<int>(gz - offset_z);

        return l2i(x + 1, y + 1, z + 1);
    }
    // Convert local coordinate to local index
    inline int l2i(int x, int y, int z) const
    {
        return static_cast<int>(x + y * (size_x + 2) + z * (size_x + 2) * (size_y + 2));
    }

private:
    void data_no_ghost_common(const std::vector<float> &data, float *data_no_ghost) const;
};

#endif
