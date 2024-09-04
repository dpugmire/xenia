rm -rf contour_v/IMG*.png
#mpirun -np 1 ./build/render --file contour_v.bp --json contour.json --input_engine SST --output contour_v/IMG.%03d.png --field V --clip 1.0 50.0 --position 9 9 9 --lookat 3.5 3.5 3.5 --imagesize 512 512 --scalar_range 0.1 0.2

mpirun -np 1 ./build/render --file contour_v.bp --json contour.json --input_engine SST --output contour_v/IMG.%03d.png --field U --clip 1.0 50.0 --position 9 9 9 --lookat 3.5 3.5 3.5 --imagesize 512 512 --scalar_range 0.0 0.7
