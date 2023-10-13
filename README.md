# OpenGL & C++ Animation Demo

Using assimp, nlohmann, glfw, glew lib.

## Sample animation

![alt text](./readme-asset//bs-sample.gif "bs sample gif")
![alt text](./readme-asset//bs2d-sample.gif "bs2D sample gif")

## Build

### Windows

visual studio (vs 2022 is tested) and msvc x86 compiler. CMake. You can replace x86 lib with x64 lib in this project to build for x64 platform.

Recommand to build with vscode and cmake extension.

### Linux

clang (clang 16 is tested, you can use other compiler), need to install glfw / glew library. Maybe later these libs will be added to project.

## Run Env

Require [VC redist](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170). Please make sure u have installed latest vc redist!

Note: run directly in visual studio might cause file path fault.

## How to use

Change `asset/config.json` with your model path & animation play options.