sudo dpkg -r virtualgl
rm ./cmake_build -rf
mkdir ./cmake_build
cd cmake_build
export CFLAGS=-m64
export CXXFLAGS=-m64
export LDFLAGS=-m64
cmake -G"Unix Makefiles" ../
make -j16
sudo make deb

echo "Installing VirtualGL .."
sudo dpkg -r virtualgl
sudo dpkg -i ./*.deb
echo "Configuring VirtualGL.."
sudo vglserver_config
