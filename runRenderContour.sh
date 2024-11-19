rm contour_energy/*.png

mpirun -np 1 ./build/service --file f2.bp --json contour.json --input_engine SST --output contour_energy/IMG.%03d.png --field energy --service render --clip 1.0 50.0 --position 9 9 9 --lookat 3.5 3.5 3.5 --imagesize 512 512 --scalar_range 0.0 0.3
