Repo for visualization services.


## Running gray-scott example
mpirun -np 1 ./build/examples/gray-scott/gray-scott ./settings-files.json

## run convert service that converts BP files to VTK.
mpirun -np 1 ./build/converter --file gs.bp --json ./fides-gray-scott.json --output OUT.%d.vtk

## generate iso contours
mpirun -np 1 ./build/contour --file gs.bp --json ./fides-gray-scott.json --output contour.bp --field V --isovals 0.15

## render frames
mpirun -np 1 ./build/render --file contour.bp --output TMP.png --field U --clip 1.0 50.0 --position 9 9 9 --lookat 3.5 3.5 3.5



Things to fix:
- writing VTK files from mpi ranks > 1
-