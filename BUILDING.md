# Building

## Prerequisites
Before following the compilation instructions, you will need to have the appropriate build software installed on your computer, including a C++ compiler and CMake. On Linux, you'll need `clang` or `g++` installed, along with the `cmake` package. On Windows, you will need to install the Visual Studio Build Tools (or just install Visual Studio Community Edition with the C++ desktop development feature checked). You will also need to download and install CMake from their website.

If you get error messages about missing packages while building, you may want to consult the compilation instructions in the SuperCollider repository for additional packages to try installing.
For example, see [`README_LINUX.md`](https://github.com/supercollider/supercollider/blob/develop/README_LINUX.md), [`README_WINDOWS.md`](https://github.com/supercollider/supercollider/blob/develop/README_WINDOWS.md), and [`README_MACOS.md`](https://github.com/supercollider/supercollider/blob/develop/README_MACOS.md).

## Cloning the Source and Compiling
Start by cloning the flexplugins repository to your computer.
```
git clone https://github.com/fleximeter/flexplugins.git
```
Next, clone the SuperCollider source code to your computer.
Note that if you don't specify `-b 3.14`, you'll get the development branch. This could be an issue,
because the plugin interface changes from time to time. To be safe, use the same
version of the SuperCollider source code as the version that you run on your computer.
```
git clone -b 3.14 --recurse-submodules https://github.com/supercollider/supercollider.git
```
Now you will need to make a build directory inside the `flexplugins` folder.
```
cd flexplugins
mkdir build
cd build
```
Run CMake in preparation for compilation. You will need the path to the SuperCollider source code for this.
```
cmake -DSC_PATH=/path/to/supercollider/directory -DSUPERNOVA=ON -DCMAKE_BUILD_TYPE=Release ..
```
### For Linux
Then you can compile the code:
```
make
```
and if you want to use multiple cores to spead up the build:
```
make -j4  # or however many cores you want to use
```
Finally, to install the plugins so you don't have to manually copy the binaries to your extensions directory:
```
make install
```
Restart SuperCollider and the plugins should be available.

### For Windows
To compile the code, run
```
cmake --build . --config Release
```
and to install the plugins so you don't have to manually copy the binaries to your extensions directory:
```
cmake --install . --config Release
```
Restart SuperCollider and the plugins should be available.
