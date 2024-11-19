rm -rf streamlines/*.png
./build/service --file streamlines.bp --json streamlines.json --input_engine SST --output streamlines/IMG.%03d.png --service render --clip 1.0 50.0 --position 9 9 9 --lookat 3.5 3.5 3.5 --imagesize 512 512 --field scalar
