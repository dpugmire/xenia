rm -rf contour_u/IMG*.png
mpirun -np 1 ./build/render --file contour_u.bp --json contour.json --input_engine SST --output contour_u/IMG.%03d.png --field U --clip 1.0 50.0 --position 9 9 9 --lookat 3.5 3.5 3.5 --imagesize 512 512 --scalar_range 0.0 0.3

#.7 .9
