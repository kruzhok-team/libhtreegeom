# The Cyberida Hierarchy Tree Geometry Library

The C++ library with the C interface for processing hierarchical
graphs with geometry. Used by the CyberiadaML library to process state
machine graphs with geometrical parameters.

The library uses homog2d geometry transformation library.

The code is distributed under the Lesser GNU Public License (version
3), the documentation -- under the GNU Free Documentation License
(version 1.3).

## Requirements

* build-essential (c++ is required)
* homog2d geometry library - https://github.com/skramm/homog2d
* cmake (version 3.12+)

## Installation

Create `build` directory: `mkdir build && cd build`

Run `cmake ..` to build the library binaries and the test program.

Run `make install` to install the library.

Use CMake parameters to change the build type / installation prefix / etc.
