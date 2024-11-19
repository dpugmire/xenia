rm -rf streamlines.bp

mpirun -np 1 ./build/service --service streamlines --file f1.bp --json ./clover-sim.json --input_engine SST --output streamlines.bp --output_engine SST --fieldx velocityX --fieldy velocityY --fieldz velocityZ --step-size 0.01 --max-steps 100 --seed-grid-bounds "1 2 1 3 1 5" --seed-grid-dims "10 10 10" --tube-size 0.05 --tube-num-sides 8
