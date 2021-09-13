/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fstream>
#include <unistd.h>
#include <sys/time.h>
#include <cstdlib>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <sys/time.h>
#include <memory>
#include <getopt.h>
#include <chrono>
#include <AmTsPlayer.h>
#include <termios.h>

#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <iostream>

using namespace std;

#define  ONLY_ONE_VIDEO 0

#if ONLY_ONE_VIDEO
const int changNum = 1;
#else
const int changNum = 2;
#endif

am_tsplayer_handle g_session[changNum] = {0};

typedef struct {
    char tsFilePatch[50];
    am_tsplayer_video_codec videoFmt;
    am_tsplayer_audio_codec audioFmt;
    int vpid;
    int apid;
    am_tsplayer_input_source_type inPutTsType;
    am_tsplayer_input_buffer_type drmmode;
    int index;
    bool run;
    bool mute;
    bool setAudio;
    pthread_t mThread;
} TsParams;

void video_callback(void *user_data, am_tsplayer_event *event)
{
    //UNUSED(user_data);
    TsParams* mTsParams = (TsParams*)user_data;
    //printf("video_callback type %d\n", event? event->type : 0);
    switch (event->type) {
        case AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED:
        {
            printf("index:%d [evt] AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED: %d x %d @%d [%d]\n",
                mTsParams->index,
                event->event.video_format.frame_width,
                event->event.video_format.frame_height,
                event->event.video_format.frame_rate,
                event->event.video_format.frame_aspectratio);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_USERDATA_AFD:
        case AM_TSPLAYER_EVENT_TYPE_USERDATA_CC:
        {
            uint8_t* pbuf = event->event.mpeg_user_data.data;
            uint32_t size = event->event.mpeg_user_data.len;
            printf("index:%d [evt] USERDATA [%d] : %x-%x-%x-%x %x-%x-%x-%x ,size %d\n",
                mTsParams->index,event->type, pbuf[0], pbuf[1], pbuf[2], pbuf[3],
                pbuf[4], pbuf[5], pbuf[6], pbuf[7], size);
            //UNUSED(pbuf);
            //UNUSED(size);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME:
        {
            printf("index:%d [evt] AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME\n",mTsParams->index);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_VIDEO:
        {
            printf("index:%d [evt] AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_VIDEO\n",mTsParams->index);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_AUDIO:
        {
            printf("index:%d [evt] AM_TSPLAYER_EVENT_TYPE_DECODE_FIRST_FRAME_AUDIO\n",mTsParams->index);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_AV_SYNC_DONE:
        {
            printf("index:%d [evt] AM_TSPLAYER_EVENT_TYPE_AV_SYNC_DONE\n",mTsParams->index);
            break;
        }
       case AM_TSPLAYER_EVENT_TYPE_INPUT_VIDEO_BUFFER_DONE:
       {
       //    printf("[evt] AM_TSPLAYER_EVENT_TYPE_INPUT_VIDEO_BUFFER_DONE,%p\n",event->event.ptr);
            break;
       }
       case AM_TSPLAYER_EVENT_TYPE_INPUT_AUDIO_BUFFER_DONE:
       {
       //    printf("[evt] AM_TSPLAYER_EVENT_TYPE_INPUT_AUDIO_BUFFER_DONE\n");
            break;
       }
        default:
            break;
    }
}

void* TsPlayerThread(void *arg) {
    TsParams* mTsParams = (TsParams*)arg;
    mTsParams->run = true;
    std::string inputTsName(mTsParams->tsFilePatch);
    am_tsplayer_handle session;
    am_tsplayer_avsync_mode avsyncMode= TS_SYNC_AMASTER;
    am_tsplayer_video_trick_mode vTrickMode = AV_VIDEO_TRICK_MODE_NONE;
    int kRwSize = 188*300;
    int kRwTimeout = 30000;
    char buf[kRwSize];
    uint64_t fsize = 0;
    ifstream file(inputTsName.c_str(), ifstream::binary);
    if (mTsParams->inPutTsType == TS_MEMORY) {
        file.seekg(0, file.end);
        fsize = file.tellg();
        if (fsize <= 0) {
            printf("file %s size %lld return\n", inputTsName.c_str(), fsize);
            return 0;
        }
        file.seekg(0, file.beg);
    }
    printf("inde:%d file name = %s is_open %d, size %lld, inPutTsType %d drmmode:%d \n",
                                                                mTsParams->index,
                                                                inputTsName.c_str(),
                                                                file.is_open(),
                                                                fsize,
                                                                mTsParams->inPutTsType,
                                                                mTsParams->drmmode);
    int x = 0, y = 0, w = 0, h = 0;
    int tunnelid = 0;
    switch (mTsParams->index) {
        case 0:
            tunnelid = 0;
            x = 0;
            y = 0;
            w = 960;
            h = 540;
            break;
        case 1:
            tunnelid = 1;
            x = 960;
            y = 0;
            w = 960;
            h = 540;
            break;
        case 2:
            tunnelid = 2;
            x = 0;
            y = 540;
            w = 960;
            h = 540;
            break;
        case 3:
            tunnelid = 3;
            x = 960;
            y = 540;
            w = 960;
            h = 540;
            break;
        default:
            break;
    }


    am_tsplayer_init_params parm = {mTsParams->inPutTsType, mTsParams->drmmode, mTsParams->index, 0};
    AmTsPlayer_create(parm, &session);
    g_session[mTsParams->index] = session;
    AmTsPlayer_setSurface(session,(void*)&tunnelid);
    AmTsPlayer_setVideoWindow(session, x, y, w, h);


    uint32_t versionM, versionL;
    AmTsPlayer_getVersion(&versionM, &versionL);
    uint32_t instanceno;
    AmTsPlayer_getInstansNo(session, &instanceno);
    AmTsPlayer_setWorkMode(session, TS_PLAYER_MODE_NORMAL);
    AmTsPlayer_registerCb(session, video_callback, (void*)mTsParams);
    AmTsPlayer_setSyncMode(session, avsyncMode);

    am_tsplayer_video_params vparm;
    vparm.codectype = mTsParams->videoFmt;
    vparm.pid = mTsParams->vpid;
    AmTsPlayer_setVideoParams(session, &vparm);
    AmTsPlayer_startVideoDecoding(session);

    am_tsplayer_audio_params aparm;
    aparm.codectype = mTsParams->audioFmt;
    aparm.pid = mTsParams->apid;
    AmTsPlayer_setAudioMute(session,false,mTsParams->mute);
    AmTsPlayer_setAudioParams(session, &aparm);
    AmTsPlayer_startAudioDecoding(session);

    AmTsPlayer_showVideo(session);
    AmTsPlayer_setTrickMode(session, vTrickMode);

    am_tsplayer_input_buffer ibuf = {TS_INPUT_BUFFER_TYPE_NORMAL, (char*)buf, 0};
    am_tsplayer_result res;
    while (mTsParams->inPutTsType)
    {
        if (file.eof()) {
            printf("file read eof index:%d\n",mTsParams->index);
            break;
        }

        file.read(buf, (int)kRwSize);
        ibuf.buf_size = kRwSize;

        int retry = 100;
        do {
            if (!mTsParams->run) {
                printf("break by quit******index:%d \n",mTsParams->index);
                break;
            }
            res = AmTsPlayer_writeData(session, &ibuf, kRwTimeout);
            if (res == AM_TSPLAYER_ERROR_RETRY ) {
                usleep(300000);
            } else {
                break;
            }
        } while(res || retry-- > 0);

        if (!mTsParams->run) {
            break;
        }
    }
    while (mTsParams->inPutTsType == TS_DEMOD &&
           mTsParams->run) {
        usleep(1000000);
    }
    mTsParams->run = false;

    printf("AmTsPlayer_stopVideoDecoding index:%d\n",mTsParams->index);
    AmTsPlayer_stopVideoDecoding(session);
    AmTsPlayer_stopAudioDecoding(session);
    printf("AmTsPlayer_release index:%d\n",mTsParams->index);
    AmTsPlayer_release(session);
    printf("AmTsPlayer_release ok index:%d\n",mTsParams->index);
    return NULL;
}

void signHandler(int iSignNo) {
    (void) iSignNo;
    return;
}

int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (!initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

#if ONLY_ONE_VIDEO
void changAudio(TsParams* mTsParams,int number) {
    bool bSetMute = false;
    bSetMute = (number == 0) ? false : true;
    if (mTsParams[0].mute != bSetMute) {
        mTsParams[0].mute = bSetMute;
        mTsParams[0].setAudio = true;
		AmTsPlayer_setAudioMute(g_session[0],false,mTsParams[0].mute);
        printf("AmTsPlayer_setAudioMute : %d \n",mTsParams[0].mute);
    }
    return;
}
#else
void changAudio(TsParams* mTsParams,int number) {
    for (int i = 0 ; i < changNum;i++) {
        if (mTsParams[i].mute == false) {
            if (i == number) {
                return;
            } else {
                mTsParams[i].mute = true;
                mTsParams[i].setAudio = true;
                AmTsPlayer_setAudioMute(g_session[i],false,mTsParams[i].mute);
                printf("index:%d AmTsPlayer_setAudioMute : %d \n",i,mTsParams[i].mute);
                break;
            }
        }
    }
   // usleep(200000);
    mTsParams[number].mute = false;
    mTsParams[number].setAudio = true;
    AmTsPlayer_setAudioMute(g_session[number],false,mTsParams[number].mute);
    printf("index:%d AmTsPlayer_setAudioMute : %d \n",number,mTsParams[number].mute);
    return;
}
#endif

void stopVideo(TsParams* mTsParams,int number) {
    printf("index:%d AmTsPlayer_stopVideoDecoding g_session:0x%x \n",number,g_session[number]);
    AmTsPlayer_stopVideoDecoding(g_session[number]);
    return;
}

void startVideo(TsParams* mTsParams,int number) {
    am_tsplayer_video_params vparm;
    vparm.codectype = mTsParams->videoFmt;
    vparm.pid = mTsParams->vpid;
    printf("index:%d AmTsPlayer_startVideoDecoding g_session:0x%x \n",number,g_session[number]);
    AmTsPlayer_setVideoParams(g_session[number], &vparm);
    AmTsPlayer_startVideoDecoding(g_session[number]);

    return;
}


void printfStatus(TsParams* mTsParams,char ch) {
    int i = 0;
    printf("\n");
    printf("******************************\n");
    printf("******* key input : %c ********\n",ch);
    printf("********  q:quit   ***********\n");
    for (i = 0 ; i < changNum;i++) {
        printf("*******%d:%d channel sound******\n",i,i);
    }
    for (i = 0 ; i < changNum;i++) {
        printf("*** number:%d  audioMute:%d  ***\n",i,mTsParams[i].mute);
    }
    printf("******************************\n");

}
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    signal(SIGINT, signHandler);
    TsParams mTsParams[changNum];
    char const* file0 = "/data/1.ts";
    int file0_len = strlen(file0);
    memcpy(mTsParams[0].tsFilePatch,file0,file0_len);
    mTsParams[0].tsFilePatch[file0_len] = '\0';
    printf("file0:%s file0_len:%d\n",file0,file0_len);
    mTsParams[0].videoFmt = AV_VIDEO_CODEC_H264;
    mTsParams[0].audioFmt = AV_AUDIO_CODEC_AAC;
    mTsParams[0].vpid = 0x100;
    mTsParams[0].apid = 0x101;
    mTsParams[0].inPutTsType = TS_MEMORY;
    mTsParams[0].drmmode = TS_INPUT_BUFFER_TYPE_NORMAL;
    mTsParams[0].index = 0;
    mTsParams[0].mThread = 0;
    mTsParams[0].run = true;
    mTsParams[0].mute = false;
    mTsParams[0].setAudio = false;


#if !ONLY_ONE_VIDEO
    char const* file1 = "/data/2.ts";
    int file1_len = strlen(file1);
    memcpy(mTsParams[1].tsFilePatch,file1,file1_len);
    mTsParams[1].tsFilePatch[file1_len] = '\0';
    mTsParams[1].videoFmt = AV_VIDEO_CODEC_H264;
    mTsParams[1].audioFmt = AV_AUDIO_CODEC_MP3;
    mTsParams[1].vpid = 0xc8;
    mTsParams[1].apid = 0xc9;
    mTsParams[1].inPutTsType = TS_MEMORY;
    mTsParams[1].drmmode = TS_INPUT_BUFFER_TYPE_NORMAL;
    mTsParams[1].index = 1;
    mTsParams[1].mThread = 0;
    mTsParams[1].run = true;
    mTsParams[1].mute = true;
    mTsParams[1].setAudio = false;
#endif
#if 0

    char const* file2 = "/data/2.ts";
    memcpy(mTsParams[2].tsFilePatch,file2,strlen(file2));
    mTsParams[2].videoFmt = AV_VIDEO_CODEC_H264;
    mTsParams[2].audioFmt = AV_AUDIO_CODEC_MP3;
    mTsParams[2].vpid = 0xc8;
    mTsParams[2].apid = 0xc9;
    mTsParams[2].inPutTsType = TS_MEMORY;
    mTsParams[2].drmmode = TS_INPUT_BUFFER_TYPE_NORMAL;
    mTsParams[2].index = 2;
    mTsParams[2].mThread = 0;
    mTsParams[2].mute = true;
    mTsParams[2].setAudio = false;

    char const* file3 = "/data/2.ts";
    memcpy(mTsParams[3].tsFilePatch,file3,strlen(file3));
    mTsParams[3].videoFmt = AV_VIDEO_CODEC_H264;
    mTsParams[3].audioFmt = AV_AUDIO_CODEC_MP3;
    mTsParams[3].vpid = 0xc8;
    mTsParams[3].apid = 0xc9;
    mTsParams[3].inPutTsType = TS_MEMORY;
    mTsParams[3].drmmode = TS_INPUT_BUFFER_TYPE_NORMAL;
    mTsParams[3].index = 3;
    mTsParams[3].mThread = 0;
    mTsParams[3].run = true;
    mTsParams[3].mute = true;
    mTsParams[3].setAudio = false;
#endif

     int i = 0;
     for (i = 0 ; i < changNum;i++) {
        if (pthread_create(&(mTsParams[i].mThread), NULL, TsPlayerThread, &mTsParams[i]) == 0) {
            printf("pthread_create %d ok %ld\n",i,mTsParams[i].mThread);
        } else {
            mTsParams[i].mThread = 0;
            printf("pthread_create %d errno:%d (%s) \n",i,errno,strerror(errno));
        }
        usleep(5000);
     }

    char ch = 0;
    bool isExit = false;
    int run_false_index_count = 0;
    while (1) {
        if (_kbhit()) {
            ch = getchar();
            printfStatus(mTsParams,ch);
            switch (ch) {
                case 'q':
                    printf("----break\n");
                    for (i = 0 ; i < 4;i++) {
                        mTsParams[i].run = false;
                    }
                    isExit = true;
                    break;
                case '0':
                    changAudio(mTsParams, 0);
                    break;
                case '1':
                    changAudio(mTsParams, 1);
                    break;
                case '2':
                    changAudio(mTsParams, 2);
                    break;
                case '3':
                    changAudio(mTsParams, 3);
                    break;
                case 'p':
                    //stop video 2 decoding,test
                    stopVideo(mTsParams,changNum-1);
                    break;
                case 'r':
                    //start video 2 decoding,test
                    startVideo(mTsParams,changNum-1);
                    break;
                default:
                    break;
            }
            ch = 0;
        }
        for (i = 0 ; i < changNum;i++) {
            if (mTsParams[i].run == false) {
                run_false_index_count++;
            }
        }
        if (run_false_index_count == changNum) {
            isExit = true;
            printf("---->all channel exit changNum:%d \n",changNum);
        }
        if (isExit) {
            break;
        }
        usleep(200000);
    }

    for (i = 0 ; i < changNum;i++) {
        printf("---->pthread_join %d \n",i);
        mTsParams[i].run = false;
        if (mTsParams[i].mThread != 0) {
            pthread_join(mTsParams[i].mThread, NULL);
            mTsParams[i].mThread = 0;
        }
    }

    printf("---->pthread_join end\n");
    return 0;
}
