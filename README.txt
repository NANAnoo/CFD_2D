
A CFD simulator in 2-D with SPH.

Features:

[o] pressure
[o] viscosity
[o] surface tension

How to build:

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
cmake --build ./ --target CFD_2D -j 16