rm ./cmake_build -rf
mkdir ./cmake_build
cd cmake_build
export CFLAGS=-m64
export CXXFLAGS=-m64
export LDFLAGS=-m64
sudo apt-get install libxv-dev libxtst-dev libglu1-mesa-dev
sudo rm /opt/libjpeg-turbo -rf
sudo cp /opt/libjpeg-turbo.64 /opt/libjpeg-turbo -rf
cmake -G"Unix Makefiles" ../
make -j16
sudo make install
