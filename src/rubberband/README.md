# LoopPhasor

This is a UGen plugin for the SuperCollider audio server. It is based on the standard `Phasor` UGen, with some modifications. `LoopPhasor` adds a trigger to play to the end of the sample, as well as loop start and end points. This allows audio samples to be sustained indefinitely provided that acceptable looping positions are specified.

## Building
### Step 1
You will need to have the SuperCollider repository cloned on your computer.
```
git clone -b 3.13 --recursive https://github.com/supercollider/supercollider.git
```
(if you're building for a different release, change the branch to something other than `3.13`)

### Step 2
Install the appropriate build tools. Consult the README files for developing SuperCollider (https://github.com/supercollider/supercollider/blob/develop/README_WINDOWS.md, etc.) You will definitely need to have CMake installed.

### Step 3
Follow these instructions from the root of the repository to compile the UGen (replace `path_to_sc_source` with the path to the SuperCollider source code on your computer):
```
mkdir build
cd build
cmake -DSC_PATH=path_to_sc_source -DCMAKE_BUILD_TYPE=RELEASE -DSUPERNOVA=ON ..
# if you're on Windows, run the following line
# cmake --build . --config Release
# if you're on another platform, run the following line
# make
```

### Step 4
Copy the .scx and .sc files to your Extensions directory (run `Platform.userExtensionDir` from `sclang` to find what this is).
