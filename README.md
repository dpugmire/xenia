Repo for visualization services.


## Running gray-scott example
mpirun -np 1 ./build/examples/gray-scott/gray-scott ./settings-files.json

## run convert service that converts BP files to VTK.
mpirun -np 1 ./build/converter --file gs.bp --json ./fides-gray-scott.json --output OUT.%d.vtk

## generate iso contours
mpirun -np 1 ./build/contour --file gs.bp --json ./fides-gray-scott.json --output contour.bp --field V --isovals 0.15

mpirun -np 1 ./build/converter --file contour.bp --output OUT.bp

## render frames
mpirun -np 1 ./build/render --file contour.bp --output TMP.png --field U --clip 1.0 50.0 --position 9 9 9 --lookat 3.5 3.5 3.5



## to run an example using SST:
Edit adios2.xml and change the engine type of SimulationOutput to "SST".


Things to fix:
- writing VTK files from mpi ranks > 1
-