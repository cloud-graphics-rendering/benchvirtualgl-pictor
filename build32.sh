rm ./cmake_build -rf
mkdir ./cmake_build
export CFLAGS=-m32
export CXXFLAGS=-m32
export LDFLAGS=-m32
sudo apt-get install g++-multilib libxv-dev:i386 libxtst-dev:i386 libglu1-mesa-dev:i386
cd cmake_build
sudo cp /opt/libjpeg-turbo.32 /opt/libjpeg-turbo -rf
cmake -G"Unix Makefiles" ../
make -j16
sudo make install
#mv ./lib ./lib32

#rm client CMakeCache.txt CMakeFiles cmake_install.cmake cmake_uninstall.cmake -rf
#export CFLAGS=-m64
#export CXXFLAGS=-m64
#export LDFLAGS=-m64
#sudo apt-get install libxv-dev libxtst-dev libglu1-mesa-dev
#cmake -G"Unix Makefiles" ../
#make
