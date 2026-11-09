# GeomLine

A C++20 desktop application for visualizing line segment intersections and comparing sequential and OpenMP algorithms.

## Features

- Uniform and clustered random segment generation in the unit square.
- Sequential all-pairs search, uniform grid partitioning, and adaptive vertical strips based on segment midpoint quantiles.
- Intersection and partition visualization with Dear ImGui, GLFW, and OpenGL.
- Timing comparisons with variable segment counts and thread counts.

## Requirements

- CMake 3.20 or newer and Git.
- A C++20 compiler with OpenMP support (for example, GCC from MSYS2 UCRT64 on Windows).
- OpenGL development libraries and a working graphical environment.
- Internet access for the first configuration to download GLFW and Dear ImGui.

On Linux, GLFW also needs development packages for the selected window system. Apple Clang requires a separately configured OpenMP runtime.

## Build and run

Run from a terminal with your compiler and build tools on PATH:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

For Ninja, add `-G Ninja` to the configure command. Run `./build/app` on Linux or `./build/app.exe` on Windows with a single-configuration generator. Visual Studio places the executable in `build/Release/app.exe`.

## Usage

Choose the segment count, distribution, and OpenMP thread count, then click **Generate Segments**. Run an individual algorithm to display its results, or use **Quick Compare** and the benchmark controls to compare timings. White lines are segments, red dots are intersections, and blue rectangles show spatial partitions.

## Result semantics and limitations

Solvers accept segments in the normalized domain `[0, 1] x [0, 1]`. Results contain one point per intersecting segment pair: several pairs can produce the same coordinate. For collinear overlap, one overlapping endpoint is returned, rather than an overlap interval. Geometry uses an absolute tolerance of `1e-9`; this is a numerical demonstration, not an exact arithmetic geometry library.

The adaptive solver partitions only along the x axis; it is not recursive coordinate bisection. Parallel output order can vary. Benchmarks run synchronously on the UI thread and report single-run wall times, including partitioning overhead, so large workloads can temporarily pause the interface. Speedup depends on data distribution and hardware.

## Layout

- `Source/geometry.*`: geometric primitives and intersection predicates.
- `Source/generator.h`: random segment generation.
- `Source/solver_*.h`: sequential, grid, and adaptive strip solvers.
- `Source/app.h`, `Source/main.cpp`: interface and application lifecycle.
- `Tests/solvers.cpp`: boundary cases and seeded comparisons between solvers.
