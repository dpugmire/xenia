Repo for visualization services.


## Running gray-scott example
mpirun -np 1 ./build/examples/gray-scott/gray-scott ./settings-files.json

## run convert service that converts BP files to VTK.
mpirun -np 1 ./build/noop --file gs.bp --json ./fides-gray-scott.json --output OUT.%d.vtk


fides/install/examples/


Things to fix:
- writing VTK files from mpi ranks > 1
-