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

SSL Support:
If built with SSL support, VirtualGL can use OpenSSL to encrypt the traffic it sends and 
receives via the VGL Transport. This is only a marginally useful feature, however, since 
VirtualGL can also tunnel the VGL Transport through SSH. To enable SSL support, set the 
VGL_USESSL CMake variable to 1. 
Please look here for more information: https://github.com/VirtualGL/virtualgl/blob/master/BUILDING.md
