# RESTFUL EZREAL

I know it doesn't rhyme

## Dependencies

Aside from the dependencies already in this project (Walnut, Imgui... All include as git submodules), we require the Vulkan SDK (OS specific), jsoncpp, curl (riot-cpp dependencies).

I will make a repo for building in Visual Studio that isn't CMake. Coming soon.

## Build Instructions

This run's fine on linux, most of these build instructions are for windows users.

Please install visual studio with c++. I'm referring to microsoft visual studio not vscode. If you are using a different IDE you are welcome to use it whilst developing with us but I;m not going to provide build instructions. You are welcome to create a pull request to improve compatability.

All of this assumes you have git already installed. I have been using git bash instead of command prompt.

### 0. Cloning the project repository.

Do not forget the --recurse-submodules add the end as the repo contains submodules so they we can maintain up to date dependencies.

```bash
# with SSH
git clone git@github.com:Dan-Tan/RestfulEzreal.git --recurse-submodules
# with HTTPS
git clone https://github.com/Dan-Tan/RestfulEzreal.git --recurse-submodules
```

### 1. Install VCPKG

To make package management easier and because I'm not too comfortable with CMAKE and Visual Studio I used VCPKG to help with install, setting environment variables and so on.

Set up Visual Studio before this.

```bash
git clone https://github.com/Microsoft/vcpkg.git # cloning the vcpkg directory

./vcpkg/bootstrap-vcpkg.sh # build vcpkg

./vcpkg integrate install # "linking" vcpkg with visual studio
```

### 2. Installing CURL and Jsoncpp

Identify the architecture of your computer

```bash
# installing curl
./vcpkg install curl --triplet=<TRIPLET>
# installing jsoncpp
./vcpkg install jsoncpp --triplet=<TRIPLET>
```

For example, 64 bit windows

```bash
# Example for 64 bit windows
./vcpkg install curl --triplet=x64-windows
./vcpkg install jsoncpp --triplet=x64-windows
```

### 3. Installing the Vulkan SDK

Vulkan should take care of environment path for you so it should be as simple as following the download instructions.

The SDK to download can be found [here](https://vulkan.lunarg.com/)

### 4. Opening with visual studio from directory.

Open visual studio and select "Open a local folder" under get started on the right.

Select the folder RestfulEzreal, this should be the directory you cloned.

Under "Project", select "Configure Cache".

Under "Build", select build all.

In the directory, "RestfulEzreal/build/x64-Debug" there will be an executable (.exe) file that is the app.

(Just "RestfuEzreal/build" if on linux)

## TODO:

Create build instructions so other people can work on it.

