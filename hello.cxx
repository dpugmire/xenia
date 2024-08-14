#include <mpi.h>
#include <stdio.h>
#include <iostream>

static void DebugWait(int rank)
{
    char    a;
    if(rank == 0)
    {
        scanf("%c", &a);
        printf("%d: Starting now\n", rank);
    } 
    MPI_Bcast(&a, 1, MPI_BYTE, 0, MPI_COMM_WORLD);
    printf("%d: Starting now\n", rank);
}

int main(int argc, char** argv)
{
    std::cout<<"Args:"<<std::endl;
    for (int i = 0; i < argc; i++)
        std::cout<<" "<<i<<": "<<argv[i]<<std::endl;

    MPI_Init(&argc, &argv);
    int rank, world;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world);
    //DebugWait(rank);


    printf("Hello: rank %d, world: %d\n",rank, world);
    MPI_Finalize();
}
