/*
 * main.cpp
 *
 *  Created on: 2014-3-2
 *      Author: xy
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <linux/videodev2.h>
//ffplay -f video4linux2 -framerate 30 -video_size hd720 /dev/video0
#ifdef __cplusplus
extern "C" {
#endif
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include <stdint.h>
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

#ifdef __cplusplus
}
#endif
//输入设备
#include "v4l2uvc.h"
int main() {
	//v4l2 val
	char *videodevice = "/dev/video0";
	int width = 640; //320;
	int height = 480; //240;
	int brightness = 0, contrast = 0, saturation = 0, gain = 0;
	int quality = 95;
	int format = V4L2_PIX_FMT_YUYV;
	struct vdIn *videoIn;
	int grabmethod = 1;

	//video encoder init
	avcodec_register_all();

	AVCodec *codec;
	AVCodecContext *c = NULL;
	int i, ret, x, y, got_output;
	FILE *f;
	AVFrame *frame;
	AVPacket pkt;
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	char filename[] = "test.264";
	printf("Encode video file %s\n", filename);

	/* find the mpeg1 video encoder */
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}
	c = avcodec_alloc_context3(codec);
	if (!c) {
		fprintf(stderr, "Could not allocate video codec context\n");
		exit(1);
	}
	/* put sample parameters */
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width = width; // 352;
	c->height = height; // 288;
	/* frames per second */
	c->time_base = (AVRational) {1,10/*25*/};
	c->gop_size = 10; /* emit one intra frame every ten frames */
	c->max_b_frames = 0; //1
	c->pix_fmt = AV_PIX_FMT_YUV422P; //v4l2是这个格式  AV_PIX_FMT_YUV420P;

	//av_opt_set(c->priv_data, "preset", "slow", 0);
	av_opt_set(c->priv_data, "tune", "zerolatency", 0); //这个可以让libav不缓存视频帧
			/********************************************************************************
			 * 有两个地方影响libav是不是缓存编码后的视频帧，也就是影响实时性：
			 * 1、av_opt_set(c->priv_data, "tune", "zerolatency", 0);这个比较主要。
			 * 2、参数中有c->max_b_frames = 1;如果这个帧设为0,就没有B帧了，编码会很快的。
			 ********************************************************************************/

	/* open it */
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec\n");
		exit(1);
	}

	f = fopen(filename, "wb");
	if (!f) {
		fprintf(stderr, "Could not open %s\n", filename);
		exit(1);
	}

	frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;

	/* the image can be allocated by any means and av_image_alloc() is
	 * just the most convenient way if av_malloc() is to be used */
	ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
			c->pix_fmt, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate raw picture buffer\n");
		exit(1);
	}

	//v4l2 init
	videoIn = (struct vdIn *) calloc(1, sizeof(struct vdIn));
	if (init_videoIn(videoIn, (char *) videodevice, width, height, format,
			grabmethod) < 0)
		exit(1);

	printf("w:%d,h:%d\n", c->width, c->height);
	time_t timep;
	timep = time(NULL);
	printf("%s\n", asctime(gmtime(&timep)));
	for (i = 0; i < 100; i++) {
		//usleep(200000);
		//从v4l2中获取数据格式为AV_PIX_FMT_YUV422P
		if (uvcGrab(videoIn) < 0) {
			fprintf(stderr, "Error grabbing\n");
			close_v4l2(videoIn);
			free(videoIn);
			exit(1);
		}
		unsigned char *yuyv = videoIn->framebuffer;
		//把数据复制到libav想要的结构中
		av_init_packet(&pkt);
		pkt.data = NULL; // packet data will be allocated by the encoder
		pkt.size = 0;
		printf("debug!!!");
		fflush(stdout);
#if 0
		/* prepare a dummy image */
		/* Y */
		for (y = 0; y < c->height; y++) {
			for (x = 0; x < c->width; x++) {
				frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
			}
		}

		/* Cb and Cr */
		for (y = 0; y < c->height/2; y++) {
			for (x = 0; x < c->width/2; x++) {
				frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
				frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
			}
		}
#else
		/* prepare  image */
		/* Y  Cb Rb */
		for (y = 0; y < c->height; y++) {
			for (x = 0; x < c->width; x++) {
				frame->data[0][y * frame->linesize[0] + x] = yuyv[y
						* frame->linesize[0] * 2 + 2 * x];
			}
		}
		/* Cb and Cr */
		for (y = 0; y < c->height; y++) {
			for (x = 0; x < c->width / 2; x++) {
				//frame->data[0][y * frame->linesize[0] + 2*x  ] = yuyv[y*frame->linesize[0]*4+4*x];
				//frame->data[0][y * frame->linesize[0] + 2*x+1] = yuyv[y*frame->linesize[0]*4+4*x+2];
				frame->data[1][y * frame->linesize[1] + x] = yuyv[y
						* frame->linesize[1] * 4 + 4 * x + 1];
				frame->data[2][y * frame->linesize[2] + x] = yuyv[y
						* frame->linesize[2] * 4 + 4 * x + 3];
			}
		}
#endif
		frame->pts = i;

		/* encode the image */
		ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
		if (ret < 0) {
			fprintf(stderr, "Error encoding frame\n");
			exit(1);
		}

		if (got_output) {
			printf("Write frame %3d (size=%5d)\n", i, pkt.size);
			fwrite(pkt.data, 1, pkt.size, f);
			av_free_packet(&pkt);
		}
		//编码
	}
	/* get the delayed frames */
	for (got_output = 1; got_output; i++) {
		fflush(stdout);

		ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
		if (ret < 0) {
			fprintf(stderr, "Error encoding frame\n");
			exit(1);
		}

		if (got_output) {
			printf("Write frame %3d (size=%5d)\n", i, pkt.size);
			fwrite(pkt.data, 1, pkt.size, f);
			av_free_packet(&pkt);
		}
	}
	timep = time(NULL);
	printf("%s\n", asctime(gmtime(&timep)));
	/* add sequence end code to have a real mpeg file */
	fwrite(endcode, 1, sizeof(endcode), f);
	fclose(f);

	avcodec_close(c);
	av_free(c);
	av_freep(&frame->data[0]);
	av_frame_free(&frame);
	printf("111\n");

	free(videoIn);
}

