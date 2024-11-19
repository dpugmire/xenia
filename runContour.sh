rm -rf f2.bp

mpirun -np 1 ./build/service --file f1.bp --json ./clover-sim.json --output f2.bp --input_engine SST --output_engine SST --service contour --sleep 0 --field pressure --isovals .5 --cell_to_point
