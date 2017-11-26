#ifndef TIFIG_HEVC
#define TIFIG_HEVC

#include "types.hpp"

extern "C" {
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
};

using namespace std;

static struct SwsContext* swsContext;

/**
 * Convert colorspace of decoded frame load into buffer
 * @param frame
 * @param dst
 * @param dst_size
 * @return the number of bytes written to dst, or a negative value on error
 */
int copyFrameInto(AVFrame* frame, uint8_t* dst, size_t dst_size)
{
    AVFrame* imgFrame = av_frame_alloc();
    int width = frame->width;
    int height = frame->height;

    uint8_t *tempBuffer = (uint8_t*) av_malloc(dst_size);

    struct SwsContext *sws_ctx = sws_getCachedContext(swsContext,
                                                      width, height, AV_PIX_FMT_YUV420P,
                                                      width, height, AV_PIX_FMT_RGB24,
                                                      0, nullptr, nullptr, nullptr);

    av_image_fill_arrays(imgFrame->data, imgFrame->linesize, tempBuffer, AV_PIX_FMT_RGB24, width, height, 1);
    uint8_t const* const* frameDataPtr = (uint8_t const* const*)frame->data;

    // Convert YUV to RGB
    sws_scale(sws_ctx, frameDataPtr, frame->linesize, 0, height, imgFrame->data, imgFrame->linesize);

    // Move RGB data in pixel order into memory
    const uint8_t* const* dataPtr = static_cast<const uint8_t* const*>(imgFrame->data);
    int size = static_cast<int>(dst_size);
    int ret = av_image_copy_to_buffer(dst, size, dataPtr, imgFrame->linesize, AV_PIX_FMT_RGB24, width, height, 1);

    av_free(imgFrame);
    av_free(tempBuffer);

    return ret;
}

/**
 * Get HEVC decoder context
 * @return
 */
AVCodecContext* getHEVCDecoderContext()
{
    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    AVCodecContext *c = avcodec_alloc_context3(codec);

    if (!c) {
        throw logic_error("Could not allocate video codec context");
    }

    if (avcodec_open2(c, c->codec, nullptr) < 0) {
        throw logic_error("Could not open codec");
    }

    return c;
}

/**
 * Decode HEVC frame and return loadable RGB data
 * @param hevcData
 */
RgbData decodeFrame(DataVector hevcData)
{
    AVCodecContext* c = getHEVCDecoderContext();
    AVFrame* frame = av_frame_alloc();

    AVPacket* packet = av_packet_alloc();
    packet->size = static_cast<int>(hevcData.size());
    packet->data = &hevcData[0];

    char* errorDescription = new char[256];

    int sent = avcodec_send_packet(c, packet);
    if (sent < 0)
    {
        av_strerror(sent, errorDescription, 256);
        cerr << "Error sending packet to HEVC decoder: " << errorDescription << endl;
        throw sent;
    }

    int success = avcodec_receive_frame(c, frame);
    if (success != 0) {
        av_strerror(success, errorDescription, 256);
        cerr << "Error decoding frame: " << errorDescription << endl;
        throw success;
    }

    size_t bufferSize = static_cast<size_t>(av_image_get_buffer_size(AV_PIX_FMT_RGB24, frame->width, frame->height, 1));

    RgbData result = {};
    result.data = (uint8_t *) malloc(bufferSize);
    result.size = bufferSize;
    result.width = frame->width;
    result.height = frame->height;

    copyFrameInto(frame, result.data, result.size);

    avcodec_close(c);
    av_free(c);
    av_free(frame);

    return result;
}

#endif