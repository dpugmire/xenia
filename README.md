Repo for visualization services.


## Running gray-scott example
mpirun -np 1 ./build/examples/gray-scott/gray-scott ./settings-files.json

## run convert service that converts BP files to VTK.
mpirun -np 1 ./build/converter --file gs.bp --json ./fides-gray-scott.json --output OUT.%d.vtk

mpirun -np 1 ./build/contour --file gs.bp --json ./fides-gray-scott.json --output contour.bp --field V --isovals 0.2


fides/install/examples/


Things to fix:
- writing VTK files from mpi ranks > 1
-