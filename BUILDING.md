# Building

## Prerequisites
Before following the compilation instructions, you will need to have the appropriate build software installed on your computer, including a C++ compiler and CMake. On Linux, it should be enough to have `clang` or `g++` installed, along with the `cmake` package. On Windows, you will need to install the Visual Studio Build Tools (or just install Visual Studio Community Edition with the C++ desktop development feature checked). You will also need to download and install CMake from their website.

There are more detailed instructions available in the README files in the SuperCollider repository. For example, [`README_LINUX.md`](https://github.com/supercollider/supercollider/blob/develop/README_LINUX.md), [`README_WINDOWS.md`](https://github.com/supercollider/supercollider/blob/develop/README_WINDOWS.md), and [`README_MACOS.md`](https://github.com/supercollider/supercollider/blob/develop/README_MACOS.md) have more information if you want to build the whole SuperCollider system from scratch.

## Cloning the Source and Compiling
Start by cloning the fplugins repository to your computer.
```
git clone https://github.com/fleximeter/fplugins.git
```
Next, clone the SuperCollider source code to your computer. Make a note of the directory location.
```
git clone -b 3.14 --recurse-submodules https://github.com/supercollider/supercollider.git
```
Now you will need to make a build directory inside the `fplugins` folder.
```
cd fplugins
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
cmake --build . --target install
```
