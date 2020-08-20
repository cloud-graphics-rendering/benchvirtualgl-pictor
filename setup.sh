#/bin/bash
sudo apt-get install libxv-dev libxtst-dev libglu1-mesa-dev freeglut3-dev
folder=/opt/libjpeg-turbo
if [ -e "$folder" ] then
	echo "libjpeg-turbo exits!"
else
	wget https://sourceforge.net/projects/libjpeg-turbo/files/2.0.0/libjpeg-turbo-official_2.0.0_amd64.deb
	sudo dpkg -i libjpeg-turbo-official_2.0.0_amd64.deb
	echo "configuration Done!"
fi

echo "If your client do not have libjpeg-turbo, please also install it on your client machine!"
