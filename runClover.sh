rm -rf f1.bp

mpirun -np 1 ./build/service --file clover-sim.bp --json ./clover-sim.json --output f1.bp --input_engine BPFile --output_engine SST --service copier --sleep 1
