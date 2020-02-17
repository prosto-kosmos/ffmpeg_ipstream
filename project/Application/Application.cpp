// Application.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "pch.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
using namespace std;
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavformat/avio.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
#include <libavutil/motion_vector.h>
#include <libavutil/frame.h>
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "postproc.lib")
#pragma comment(lib, "swresample.lib")

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

vector<bool> used;

void SaveStream(AVPacket packet, int k, string type) {
	stringstream ss;
	ss << k;
	string fn = "Stream_" + type + "_" + ss.str() + ".ips";
	if (used[k] == false) {
		FILE *fp;
		if ((fp = fopen(fn.c_str(), "wb")) == NULL) {
			printf("Cannot open file.");
			return;
		}
		fwrite("IP_STREAM", sizeof("IP_STREAM") - 1, 1, fp);
		fclose(fp);
		used[k] = true;
	}

	FILE *fp;
	if ((fp = fopen(fn.c_str(), "ab")) == NULL) {
		printf("Cannot open file.");
		return;
	}
	int new_packet_size = packet.size;
	fwrite((char*)&new_packet_size, 4, 1, fp);
	fwrite((char*)&packet.data[0], 1, packet.size, fp);
	fclose(fp);
}

int main(int argc, char *argv[]) {
	AVFormatContext   *pFormatCtx = NULL;
	vector<int>       indexVideo, indexAudio;
	AVPacket          packet;

	if (argc < 2) {
		printf("Please provide a movie file\n");
		return -1;
	}

	// Регистрируем все форматы и кодеки
	av_register_all();

	// Пробуем открыть видео файл
	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0) {
		printf("File no open\n");
		return -1; // Не могу открыть файл
	}

	// Получаем индексы видеопотоков
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			indexVideo.push_back(i);
			//cout << "Stream " << i << " is video\n";
		}

	// Получаем индексы аудиопотоков
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			indexAudio.push_back(i);
			//cout << "Stream " << i << " is audio\n";
		}

	if (indexAudio.empty() && indexVideo.empty()) {
		cout << "Не найдено ни одного видио- или аудиопотока";
		return -1; // Не нашли
	}

	used = vector<bool>(indexVideo.size() + indexAudio.size(), false);

	//Считываем информацию из пакетов и записываем в соответствующий файл
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		// Что это за пакет?
		for (int k = 0; k < indexVideo.size(); k++) {
			if (packet.stream_index == indexVideo[k]) {
				SaveStream(packet, indexVideo[k], "video");
			}
		}
		for (int k = 0; k < indexAudio.size(); k++) {
			if (packet.stream_index == indexAudio[k]) {
				SaveStream(packet, indexAudio[k], "audio");
			}
		}
		av_free_packet(&packet);
	}
	avformat_close_input(&pFormatCtx);
	return 0;
}
