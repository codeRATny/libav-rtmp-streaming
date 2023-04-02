#ifndef _STREAMER_H_
#define _STREAMER_H_

// Import STD
#include <iostream>
#include <chrono>
#include <vector>

// Import LibAV
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
};

class Streamer
{
public:
  Streamer(char *videoFileName,
           char *rtmpServerAdress, size_t streams);
  ~Streamer();
  int Stream(size_t streams);

  AVOutputFormat *ofmt = NULL;
  AVFormatContext *ifmt_ctx = NULL;
  AVFormatContext *ofmt_ctx = NULL;
  AVPacket pkt1;
  AVFormatContext **out;
  AVStream **flv_streams;

private:
  int setupInput();
  int setupOutput(size_t streams);

  int ret;

  // Input file and RTMP server address
  char *_videoFileName;
  char *_rtmpServerAdress;

protected:
  int videoIndex = -1;
  int frameIndex = 0;
  int64_t startTime = 0;
};
#endif