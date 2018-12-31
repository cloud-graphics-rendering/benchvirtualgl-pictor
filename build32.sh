rm ./cmake_build -rf
mkdir ./cmake_build
export CFLAGS=-m32
export CXXFLAGS=-m32
export LDFLAGS=-m32
sudo apt-get install g++-multilib libxv-dev:i386 libxtst-dev:i386 libglu1-mesa-dev:i386
cd cmake_build
sudo rm /opt/libjpeg-turbo -rf
sudo cp /opt/libjpeg-turbo.32 /opt/libjpeg-turbo -rf
cmake -G"Unix Makefiles" ../
make -j16
sudo make deb

