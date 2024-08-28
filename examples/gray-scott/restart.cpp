/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */

#include "restart.h"

#include <iostream>

static bool firstCkpt = true;

void WriteCkpt(MPI_Comm comm, const int step, const Settings &settings, const GrayScott &sim,
               adios2::IO io)
{
    int rank, nproc;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);
    std::cout << "checkpoint at step " << step << " create file " << settings.checkpoint_output
              << std::endl;
    adios2::Engine writer = io.Open(settings.checkpoint_output, adios2::Mode::Write);
    if (writer)
    {
        adios2::Variable<float> var_u;
        adios2::Variable<float> var_v;
        adios2::Variable<int> var_step;

        if (firstCkpt)
        {
            size_t X = sim.size_x + 2;
            size_t Y = sim.size_y + 2;
            size_t Z = sim.size_z + 2;
            size_t R = static_cast<size_t>(rank);
            size_t N = static_cast<size_t>(nproc);

            var_u = io.DefineVariable<float>("U", {N, X, Y, Z}, {R, 0, 0, 0}, {1, X, Y, Z});
            var_v = io.DefineVariable<float>("V", {N, X, Y, Z}, {R, 0, 0, 0}, {1, X, Y, Z});

            var_step = io.DefineVariable<int>("step");
            firstCkpt = false;
        }
        else
        {
            var_u = io.InquireVariable<float>("U");
            var_v = io.InquireVariable<float>("V");
            var_step = io.InquireVariable<int>("step");
        }

        writer.Put<int>(var_step, &step);
        writer.Put<float>(var_u, sim.u_ghost().data());
        writer.Put<float>(var_v, sim.v_ghost().data());

        writer.Close();
    }
}

int ReadRestart(MPI_Comm comm, const Settings &settings, GrayScott &sim, adios2::IO io)
{
    int step = 0;
    int rank, nproc;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);
    if (!rank)
    {
        std::cout << "restart from file " << settings.restart_input << std::endl;
    }
    adios2::Engine reader = io.Open(settings.restart_input, adios2::Mode::ReadRandomAccess);
    if (reader)
    {
        adios2::Variable<int> var_step = io.InquireVariable<int>("step");
        adios2::Variable<float> var_u = io.InquireVariable<float>("U");
        adios2::Variable<float> var_v = io.InquireVariable<float>("V");
        size_t X = sim.size_x + 2;
        size_t Y = sim.size_y + 2;
        size_t Z = sim.size_z + 2;
        size_t R = static_cast<size_t>(rank);
        std::vector<float> u, v;

        var_u.SetSelection({{R, 0, 0, 0}, {1, X, Y, Z}});
        var_v.SetSelection({{R, 0, 0, 0}, {1, X, Y, Z}});
        reader.Get<int>(var_step, step);
        reader.Get<float>(var_u, u);
        reader.Get<float>(var_v, v);
        reader.Close();

        if (!rank)
        {
            std::cout << "restart from step " << step << std::endl;
        }
        sim.restart(u, v);
    }
    else
    {
        std::cout << "    failed to open file " << std::endl;
    }
    return step;
}
