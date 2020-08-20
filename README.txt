# benchvirtualgl

How to build VirtualGL:
Reference: https://github.com/VirtualGL/virtualgl/blob/master/BUILDING.md
Dependence: 
32 bits:
apt-get install g++-multilib libxv-dev:i386 libxtst-dev:i386 libglu1-mesa-dev:i386

64 bits:
apt-get install libxv-dev libxtst-dev libglu1-mesa-dev

The following procedure will build the VirtualGL Client and, 
on Linux and other Un*x variants (except Mac), the VirtualGL Server components. 
On most 64-bit systems (Solaris being a notable exception), this will build a 64-bit 
version of VirtualGL. See "Build Recipes" for specific instructions on how to build 
a 32-bit or 64-bit version of VirtualGL on systems that support both.

32-bit Build on 64-bit Linux/Unix (including Mac)
Use export/setenv to set the following environment variables before running CMake:
CFLAGS=-m32
CXXFLAGS=-m32
LDFLAGS=-m32
64-bit Build on Solaris
Use export/setenv to set the following environment variables before running CMake:
CFLAGS=-m64
CXXFLAGS=-m64
LDFLAGS=-m64


Start Building:
cd {build_directory}
cmake -G"Unix Makefiles" [additional CMake flags] {source_directory}
make

Add -DCMAKE_BUILD_TYPE=Debug to the CMake command line.

Then, you need to install and config virtualGl:
$ cd build
$ rm ./*.deb
$ sudo make deb
$ sudo dpkg -r virtualgl
$ sudo dpkg -i ./*.deb
$ sudo vglserver_config

Choose 1
Then, N,N,N,X

Attention: we have scripts that describes how to compile 64 bit and 32 bit at the same time. But, we encourage you to use 64 bits
virtualGL, because all of our benchmarks are 64 bits.
---------------------------------------------------------------------------------------------------------------------------
In fact, you can foget all the steps above, and just run the two scripts: setup.sh and build64.sh.
setup.sh: only run once.
build64.sh: build and install VirtualGL on your machine automatically.
