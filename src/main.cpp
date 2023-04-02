#include <iostream>

#include "streamer.h"

#define STREAMS 100

int main(int argc, char *argv[])
{
	Streamer streamer("sample-mp4-file.mp4", "rtmp://192.168.0.13:1935/stream", STREAMS);

	return streamer.Stream(STREAMS);
}