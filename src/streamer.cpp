#include "streamer.h"

Streamer::Streamer(char *videoFileName, char *rtmpServerAdress, size_t streams)
{
    av_register_all();
    avformat_network_init();

    _videoFileName = strdup(videoFileName);
    _rtmpServerAdress = strdup(rtmpServerAdress);

    std::cout << _videoFileName << std::endl;
    std::cout << _rtmpServerAdress << std::endl;

    ret = setupInput();
    if (ret < 0)
    {
        std::cout << "video crash\n";
        return;
    }

    ret = setupOutput(streams);
    if (ret < 0)
    {
        std::cout << " crash\n";
        return;
    }
}

Streamer::~Streamer()
{
}

int Streamer::setupInput()
{
    if ((ret = avformat_open_input(&ifmt_ctx, _videoFileName, 0, 0)) < 0)
    {
        printf("Could not open input file.");
        return -1;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
    {
        printf("Failed to retrieve input stream information");
        return -1;
    }

    for (int i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoIndex = i;
            break;
        }
    }

    av_dump_format(ifmt_ctx, 0, _videoFileName, 0);
}

int Streamer::setupOutput(size_t streams)
{
    out = (AVFormatContext **)malloc(sizeof(AVFormatContext *) * streams);
    flv_streams = (AVStream **)malloc(sizeof(AVStream *) * streams);

    auto get_url = [this](size_t num)
    {
        return std::string(_rtmpServerAdress) + "_" + std::to_string(num);
    };

    for (size_t i = 0; i < streams; i++)
    {
        avformat_alloc_output_context2(&out[i], NULL, "flv", get_url(i).c_str());

        if (!out[0])
        {
            printf("Could not create output context\n");
            ret = AVERROR_UNKNOWN;
            return -1;
        }
    }

    ofmt = out[0]->oformat;

    for (int i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        AVStream *in_stream = ifmt_ctx->streams[i];
        for (size_t j = 0; j < streams; j++)
        {
            flv_streams[j] = avformat_new_stream(out[j], in_stream->codec->codec);

            if (!flv_streams[j])
            {
                printf("Failed allocating output stream\n");
                ret = AVERROR_UNKNOWN;
                return -1;
            }

            if (!flv_streams[j])
            {
                printf("Failed allocating output stream\n");
                ret = AVERROR_UNKNOWN;
                return -1;
            }
            // Copy the settings of AVCodecContext
            ret = avcodec_copy_context(flv_streams[j]->codec, in_stream->codec);
            if (ret < 0)
            {
                printf("Failed to copy context from input to output stream codec context\n");
                return -1;
            }

            ret = avcodec_copy_context(flv_streams[j]->codec, in_stream->codec);
            if (ret < 0)
            {
                printf("Failed to copy context from input to output stream codec context\n");
                return -1;
            }
            flv_streams[j]->codec->codec_tag = 0;

            if (out[0]->oformat->flags & 1 << 22)
                flv_streams[j]->codec->flags |= 1 << 22;
        }
    }
}

int Streamer::Stream(size_t streams)
{
    auto get_url = [this](size_t num)
    {
        return std::string(_rtmpServerAdress) + "_" + std::to_string(num);
    };

    std::cout << "1" << std::endl;

    for (size_t i = 0; i < streams; i++)
    {
        std::cout << get_url(i) << std::endl;
        av_dump_format(out[i], 0, get_url(i).c_str(), 1);

        if (!(ofmt->flags & AVFMT_NOFILE))
        {
            std::cout << get_url(i) << std::endl;
            ret = avio_open(&out[i]->pb, get_url(i).c_str(), AVIO_FLAG_WRITE);
            if (ret < 0)
            {
                printf("Could not open output URL '%s'", get_url(i));
                return -1;
            }
        }

        ret = avformat_write_header(out[i], NULL);
        if (ret < 0)
        {
            printf("Error occurred when opening output URL\n");
            return -1;
        }
    }

    startTime = av_gettime();
    while (1)
    {
        AVStream *in_stream, *out_stream1, *out_stream2;
        ret = av_read_frame(ifmt_ctx, &pkt1);
        if (ret < 0)
            break;

        if (pkt1.pts == AV_NOPTS_VALUE)
        {
            // Write PTS
            AVRational time_base1 = ifmt_ctx->streams[videoIndex]->time_base;
            // Duration between 2 frames (us)
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(ifmt_ctx->streams[videoIndex]->r_frame_rate);
            // Parameters
            pkt1.pts = (double)(frameIndex * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
            pkt1.dts = pkt1.pts;
            pkt1.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
        }

        if (pkt1.stream_index == videoIndex)
        {
            AVRational time_base = ifmt_ctx->streams[videoIndex]->time_base;
            AVRational time_base_q = {1, AV_TIME_BASE};
            int64_t pts_time = av_rescale_q(pkt1.dts, time_base, time_base_q);
            int64_t now_time = av_gettime() - startTime;
            if (pts_time > now_time)
                av_usleep(pts_time - now_time);
        }

        in_stream = ifmt_ctx->streams[pkt1.stream_index];
        out_stream1 = out[0]->streams[pkt1.stream_index];

        pkt1.pts = av_rescale_q_rnd(pkt1.pts, in_stream->time_base, out_stream1->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt1.dts = av_rescale_q_rnd(pkt1.dts, in_stream->time_base, out_stream1->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt1.duration = av_rescale_q(pkt1.duration, in_stream->time_base, out_stream1->time_base);

        pkt1.pos = -1;
        if (pkt1.stream_index == videoIndex)
        {
            frameIndex++;
        }

        for (size_t j = 0; j < streams; j++)
        {
            ret = av_write_frame(out[j], &pkt1);
            if (ret < 0)
            {
                printf("Error muxing packet\n");
                break;
            }
        }

        av_free_packet(&pkt1);
    }

    for (size_t j = 0; j < streams; j++)
    {
        av_write_trailer(out[j]);

        if (out[j] && !(ofmt->flags & AVFMT_NOFILE))
            avio_close(out[j]->pb);

        avformat_free_context(out[j]);
        if (ret < 0 && ret != AVERROR_EOF)
            printf("Error occurred.\n");
        return -1;
    }

    avformat_close_input(&ifmt_ctx);

    return 0;
}