#This file is for configuring both 32 bit and 64 bit VirtualGL

echo "start. removing files.."
sudo rm /usr/lib32/libdlfaker.so
sudo rm /usr/lib32/libvglfaker.so
sudo rm /usr/lib32/libgefaker.so
sudo rm /usr/lib32/libvglfaker-nodl.so

rm ./cmake_build -rf
mkdir ./cmake_build
cd cmake_build

echo "building 64 bit VirtualGL.."
sudo rm /opt/libjpeg-turbo -rf
sudo cp /opt/libjpeg-turbo.64 /opt/libjpeg-turbo -rf
export CFLAGS=-m64
export CXXFLAGS=-m64
export LDFLAGS=-m64
sudo apt-get install libxv-dev libxtst-dev libglu1-mesa-dev
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug ../
make -j16
sudo make deb
sudo mv ./lib ./lib64

echo "building 32 bit VirtualGL.."
sudo rm client CMakeCache.txt CMakeFiles cmake_install.cmake cmake_uninstall.cmake install_manifest.txt Makefile -rf
sudo rm /opt/libjpeg-turbo -rf
sudo cp /opt/libjpeg-turbo.32 /opt/libjpeg-turbo -rf
export CFLAGS=-m32
export CXXFLAGS=-m32
export LDFLAGS=-m32
sudo apt-get install g++-multilib libxv-dev:i386 libxtst-dev:i386 libglu1-mesa-dev:i386 libxi-dev:i386
cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug ../
make -j16
sudo mv ./lib ./lib32

echo "Installing VirtualGL.."
sudo dpkg -r virtualgl
sudo dpkg -i ./*.deb
echo "Configuring VirtualGL.."
sudo vglserver_config

echo "Configuring VirtualGL.."
sudo cp ./lib32/lib*.so /usr/lib32/

sudo rm /opt/libjpeg-turbo -rf
sudo cp /opt/libjpeg-turbo.64 /opt/libjpeg-turbo -rf
sudo apt-get install libxv-dev libxtst-dev libglu1-mesa-dev

# launch VNC session
#~/Documents/workspace/benchvnc-build/bin/vncserver -kill :1
#~/Documents/workspace/benchvnc-build/bin/vncserver -3dwm
#~/Documents/workspace/benchvnc-build/bin/vncviewer 127.0.0.1:5901

