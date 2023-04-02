
all:
	  g++ -g -std=c++11 -L/files/src -L/opt/ffmpeg/lib -I/opt/ffmpeg/include/ src/streamer.cpp src/main.cpp -lavcodec -lavformat -lavfilter -lavdevice -lswresample -lswscale -lavutil -o build/streamer