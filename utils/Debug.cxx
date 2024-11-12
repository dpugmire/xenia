#include "Debug.h"
#include <iostream>
//#include <chrono>
//#include <thread>
#include <unistd.h>

#ifdef ENABLE_MPI
#include <mpi.h>
#endif


void InitDebug()
{
#ifndef NDEBUG
#ifdef ENABLE_MPI
    int rank, nRanks;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nRanks);
    if (nRanks == 1)
        return;

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    for (int r = 0; r < nRanks; r++)
    {
        if (r == rank)
            std::cout<<"Rank: "<<r<<" pid= "<<getpid()<<std::endl;
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    int doLoop = (rank == 0);
    //for (int i = 0; i < 5; i++)
    while (doLoop)
    {
        sleep(1);
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        doLoop = false;
    }
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0)
        std::cout<<"Ready to go!"<<std::endl;
#endif
#endif
}