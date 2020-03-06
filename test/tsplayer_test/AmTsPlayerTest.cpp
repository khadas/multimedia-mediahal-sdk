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

#include <AmTsPlayerSession.h>
#include <AmTsPlayerTest.h>
#include "h264string.cpp"
#include "h265string.cpp"
#include "mpeg2string.cpp"

using namespace std;

TestEnv gEnv;

const int kRwSize = 188*1024;
const int kRwTimeout = 30000;
const int kPlayMaxSeconds = 3600;

void video_callback(void *user_data, am_tsplayer_event *event)
{
    UNUSED(user_data);
    printf("video_callback");
	switch (event->type) {
        case AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED:
        {
            TLog("[evt] AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED: %d x %d @%d\n",
                event->event.video_format.frame_width,
                event->event.video_format.frame_height,
                event->event.video_format.frame_rate);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_MPEG_USERDATA:
        {
            uint8_t* pbuf = event->event.mpeg_user_data.data;
            uint32_t size = event->event.mpeg_user_data.len;
            printf("[evt] AM_TSPLAYER_EVENT_TYPE_MPEG_USERDATA: %x-%x-%x-%x ,size %d\n",
                pbuf[0], pbuf[1], pbuf[2], pbuf[3], size);
            UNUSED(pbuf);
            UNUSED(size);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME:
        {
            printf("[evt] AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME\n");
            break;
        }
        default:
            break;
	}
}

#if ANDROID_PLATFORM_SDK_VERSION >= 29
const int32_t kPlaySleep = 10*1000*1000;
const int32_t kPlaytime0_2S = 200*1000;
const int32_t kPlaytime1S = 1*1000*1000;
const int32_t kPlaytime5S = 5*1000*1000;
const int32_t kPlaytime10S = 10*1000*1000;
const int32_t kAlmostRange = 2*1000*1000;

const am_tsplayer_video_info kDefaultStreamVInfo = {1920, 1080, 24, 1000000, 1};
const am_tsplayer_audio_info kDefaultStreamAInfo = {48000, 2, 2, 192000};


const struct {
    std::string tsname;
    am_tsplayer_video_codec vcodec;
    int32_t vpid;
    am_tsplayer_audio_codec acodec;
    int32_t apid;
} kTsToPlay[] = {
    {"264_1080.ts",     AV_VIDEO_CODEC_H264, 0x100, AV_AUDIO_CODEC_AAC, 0x101},
    {"264_720.ts",      AV_VIDEO_CODEC_H264, 0x100, AV_AUDIO_CODEC_AAC, 0x101},
};
const size_t kNumTsToPlay =
            sizeof(kTsToPlay) / sizeof(kTsToPlay[0]);

#define EXPECT_ALMOST(A, B)       \
  EXPECT_GE(A + kAlmostRange, B); \
  EXPECT_LE(A - kAlmostRange, B);


class playbackFromMemory : public testing::Test {
    public:
    std::shared_ptr<AmTsPlayerSession>s;
    std::shared_ptr<AmTsPlayerSession::AVSync>av;
    std::shared_ptr<AmTsPlayerSession::Control>ctl;
    std::shared_ptr<AmTsPlayerSession::Video>v;
    std::shared_ptr<AmTsPlayerSession::Audio>a;
    std::shared_ptr<AmTsPlayerSession::AD>ad;
    std::shared_ptr<AmTsPlayerSession::Subtitle>sub;
    char* mBuf;
    std::thread mThread;
    std::atomic_bool mRunning;
    std::ifstream mFile;
    std::mutex mEofLock;
    std::condition_variable mEofCondition;
    const int kRwSize = 188*1024;
    TestEnv mEnv;
    int32_t mSrcIdx = -1;

    virtual void SetUp(){
        s = std::make_shared<AmTsPlayerSession>(
                    video_callback,
                    TS_MEMORY,
                    TS_PLAYER_MODE_NORMAL);
        av = s->mAVSync;
        ctl = s->mControl;
        v = s->mVideo;
        a = s->mAudio;
        ad = s->mAD;
        sub = s->mSubtitle;

        GetEnv();
        string ifile = mEnv.inputTsDir +  mEnv.inputTsName;
        printf("dir = %s, name = %s, ifile = %s\n",
                mEnv.inputTsDir.c_str(), mEnv.inputTsName.c_str(), ifile.c_str());
        mFile.open(ifile.c_str(), ifstream::binary);
        mBuf = new char[kRwSize];
        mFile.seekg(0, mFile.end);
        long fsize = mFile.tellg();
        ASSERT_GT(fsize, 0);
        //EXPECT_GT(fsize, 0);

        mFile.seekg(0, mFile.beg);
        mRunning.store(true);

        av->setSyncMode(mEnv.avsyncMode);
        av->setPcrPid(mEnv.pcrPid);
        am_tsplayer_video_params vparm;
        vparm.codectype = mEnv.vCodec;
        vparm.pid = mEnv.vPid;
        v->setParams(&vparm);
        v->startDecoding();
        am_tsplayer_audio_params aparm;
        aparm.codectype = mEnv.aCodec;
        aparm.pid = mEnv.aPid;
        a->setParams(&aparm);
        a->startDecoding();
        ctl->setTrickMode(AV_VIDEO_TRICK_MODE_NONE);

        mThread = std::thread([this, fsize]() {
            am_tsplayer_input_buffer ibuf = {TS_INPUT_BUFFER_TYPE_NORMAL, (char*)mBuf, 0};
            long pos = 0;
            UNUSED(fsize);
            while (mRunning.load())
            {
                usleep(20000);
                if (mFile.eof()) {
                    TLog("file eof\n");
                    std::unique_lock <std::mutex> l(mEofLock);
                    mEofCondition.notify_all();
                    break;
                }
                mFile.read(mBuf, (int)kRwSize);
                ibuf.buf_size = kRwSize;
                pos += kRwSize;
                //TLog("size(%ld), pos %ld\n", fsize, pos);
                am_tsplayer_result res = s->writeData(&ibuf, kRwTimeout);
                if (AM_TSPLAYER_ERROR_RETRY == res) {
                    usleep(100000);
                    continue;
                } else if (res < 0) {
                    printf("writeData res %d break\n", res);
                    break;
                }
            }
        });
    }

    virtual void TearDown(){
        mRunning.store(false);
        if (mThread.joinable())
            mThread.join();
        delete [](mBuf);
        if (mFile.is_open())
            mFile.close();
        TLog("TearDown\n");
    }

    void GetEnv() {
        memcpy(&mEnv, &gEnv, sizeof(gEnv));
        if (mSrcIdx >= 0) {
            mEnv.vCodec = kTsToPlay[mSrcIdx].vcodec;
            mEnv.vPid = kTsToPlay[mSrcIdx].vpid;
            mEnv.aCodec = kTsToPlay[mSrcIdx].acodec;
            mEnv.aPid = kTsToPlay[mSrcIdx].apid;
            mEnv.inputTsName.assign(kTsToPlay[mSrcIdx].tsname);
        }
    }

    void setSrcIdx(int32_t idx) {
        mSrcIdx = idx;
        printf("setSrcIdx idx %d\n", idx);
    }

    void Playsleep(uint32_t us)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }
};

class playbackTs : public playbackFromMemory,
                   public testing::WithParamInterface<int32_t> {
    public:
    virtual void SetUp() override {
        playbackFromMemory::setSrcIdx(GetParam());
        playbackFromMemory::SetUp();
    }
};

TEST_P(playbackTs, tenSec)
{
    Playsleep(kPlaytime10S);
}

INSTANTIATE_TEST_SUITE_P(playbackFromMemory, playbackTs,
        testing::Range(0, (int)kNumTsToPlay));


uint32_t gExpectEvent;
bool gGotEvent;
void callback(void *user_data, am_tsplayer_event *event)
{
    UNUSED(user_data);
    ASSERT_NE((void*)event, nullptr);
    if ((uint32_t)event->type == gExpectEvent) {
        gGotEvent = true;

        if (event->type == AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED) {
            video_format_t video_format = event->event.video_format;
            EXPECT_GT(video_format.frame_width, 0);
            EXPECT_LE(video_format.frame_width, 4096);
            EXPECT_GT(video_format.frame_height, 0);
            EXPECT_LE(video_format.frame_height, 4096);
            EXPECT_GT(video_format.frame_rate, 0);
            EXPECT_LE(video_format.frame_rate, 120);
        }
    }
    printf("test callback mExpectEvent %d, current type %d\n",
        gExpectEvent, event->type);
}

class playbackEvent : public playbackFromMemory {
    public:

    virtual void SetUp() override {
        gGotEvent = false;
        playbackFromMemory::SetUp();
        s->register_cb(callback, nullptr);
    }
};

TEST_F(playbackEvent, firstFrameToggle)
{
    printf("firstFrameToggle\n");
    gExpectEvent = AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME;
    Playsleep(kPlaytime1S);
    EXPECT_EQ(gGotEvent, true);
}

TEST_F(playbackEvent, videoFormatChange)
{
    printf("videoFormatChange\n");
    gExpectEvent = AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED;
    Playsleep(kPlaytime1S);
    EXPECT_EQ(gGotEvent, true);
}


TEST_F(playbackFromMemory, DISABLED_playForever)
{
    while(1)
        Playsleep(kPlaytime10S);
}

TEST_F(playbackFromMemory, playFileMax600S)
{
    if (mFile.is_open()) {
        std::unique_lock <std::mutex> l(mEofLock);
        mEofCondition.wait_for(l, chrono::seconds(600));
    }
}

TEST_F(playbackFromMemory, play10S)
{
    Playsleep(kPlaytime10S);
}


TEST_F(playbackFromMemory, startStopVideo)
{
    v->startDecoding();
    Playsleep(kPlaytime5S);
    v->stopDecoding();
    Playsleep(kPlaytime5S);
}

TEST_F(playbackFromMemory, stopStartVideo)
{
    Playsleep(kPlaytime1S);
    v->stopDecoding();
    Playsleep(kPlaytime5S);
    v->startDecoding();
    Playsleep(kPlaytime5S);
}

TEST_F(playbackFromMemory, pauseResumeVideo)
{
    Playsleep(kPlaytime5S);
    v->pauseDecoding();
    Playsleep(kPlaytime5S);
    v->resumeDecoding();
    Playsleep(kPlaytime5S);
}

TEST_F(playbackFromMemory, startStopAudio)
{
    a->startDecoding();
    Playsleep(kPlaytime5S);
    a->stopDecoding();
    Playsleep(kPlaytime5S);
}

TEST_F(playbackFromMemory, pauseResumeAudio)
{
    Playsleep(kPlaytime10S);
    a->pauseDecoding();
    Playsleep(kPlaytime10S);
    a->resumeDecoding();
    Playsleep(kPlaytime10S);
}

#define CLASS_PMEMORY_P(name, parmtype)                     \
class name : public playbackFromMemory,                     \
             public testing::WithParamInterface<parmtype> { \
    public:                                                 \
    virtual void SetUp() {                                  \
        playbackFromMemory::SetUp();                        \
        parm = GetParam();                                  \
    }                                                       \
    protected:                                              \
        parmtype parm;                                      \
};

CLASS_PMEMORY_P(AVSyncMode, am_tsplayer_avsync_mode);
TEST_P(AVSyncMode, syncmode)
{
    EXPECT_EQ(av->setSyncMode(parm), 0);
    am_tsplayer_avsync_mode mode;
    EXPECT_EQ(av->getSyncMode(&mode), 0);
    EXPECT_EQ(mode, parm);
    Playsleep(kPlaySleep);
}

INSTANTIATE_TEST_SUITE_P(playbackFromMemory, AVSyncMode,
        testing::Values(TS_SYNC_AMASTER, TS_SYNC_VMASTER,
                TS_SYNC_PCRMASTER, TS_SYNC_NOSYNC));

CLASS_PMEMORY_P(AVSyncTime, float);
TEST_P(AVSyncTime, fastspeed)
{
    int64_t ctime, dtime;
    EXPECT_EQ(av->getCurrentTime(&ctime), 0); //initial 0
    EXPECT_EQ(av->getDelayTime(&dtime), 0); // after 10s
    EXPECT_EQ(ctime, 0);
    EXPECT_EQ(dtime, 0);
    Playsleep(kPlaySleep);
    EXPECT_EQ(av->getCurrentTime(&ctime), 0); // after 10s
    EXPECT_EQ(av->getDelayTime(&dtime), 0); // after 10s
    EXPECT_ALMOST(ctime, kPlaySleep*parm);
    EXPECT_ALMOST(dtime, kPlaySleep*parm);
}

INSTANTIATE_TEST_SUITE_P(playbackFromMemory, AVSyncTime,
         testing::Values(0.5, 1, 2, 4));

TEST_F(playbackFromMemory, bufferStat)
{
    Playsleep(kPlaytime10S);

    am_tsplayer_buffer_stat stat;
    EXPECT_EQ(ctl->getBufferStat(TS_STREAM_VIDEO, &stat), 0);
    EXPECT_GE(stat.size, 0);
    EXPECT_EQ(stat.size, stat.data_len + stat.free_len);
    EXPECT_EQ(ctl->getBufferStat(TS_STREAM_AUDIO, &stat), 0);
    EXPECT_EQ(stat.size, stat.data_len + stat.free_len);
    EXPECT_EQ(ctl->getBufferStat(TS_STREAM_AD, &stat), 0);
    EXPECT_EQ(stat.size, stat.data_len + stat.free_len);
    EXPECT_EQ(ctl->getBufferStat(TS_STREAM_SUB, &stat), 0);
    EXPECT_EQ(stat.size, stat.data_len + stat.free_len);
}

TEST_F(playbackFromMemory, TrickNone)
{
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_NONE), 0);
    Playsleep(kPlaytime5S);
}

TEST_F(playbackFromMemory, Trickfffb)
{
    for (int i = 0; i < 50; i++) {
        EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_PAUSE_NEXT), 0);
        Playsleep(kPlaytime0_2S);
    }
}

TEST_F(playbackFromMemory, TrickI)
{
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_IONLY), 0);
    Playsleep(kPlaytime5S);
}

TEST_F(playbackFromMemory, TrickItofffb)
{
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_IONLY), 0);
    Playsleep(kPlaytime5S);
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_NONE), 0);
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_PAUSE_NEXT), 0);
    Playsleep(kPlaytime5S);
}

TEST_F(playbackFromMemory, TrickfffbtoI)
{
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_PAUSE_NEXT), 0);
    Playsleep(kPlaytime5S);
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_NONE), 0);
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_IONLY), 0);
    Playsleep(kPlaytime5S);
}

TEST_F(playbackFromMemory, TrickItonone)
{
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_IONLY), 0);
    Playsleep(kPlaytime5S);
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_NONE), 0);
    Playsleep(kPlaytime5S);
}

TEST_F(playbackFromMemory, Trickfffbtonone)
{
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_PAUSE_NEXT), 0);
    Playsleep(kPlaytime5S);
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_NONE), 0);
    Playsleep(kPlaytime5S);
}

TEST_F(playbackFromMemory, videoShowHide)
{
    Playsleep(kPlaytime10S);
    EXPECT_EQ(v->setHide(), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(v->setShow(), 0);
    Playsleep(kPlaytime10S);
}

CLASS_PMEMORY_P(VideoMatchMode, am_tsplayer_video_match_mode);
TEST_P(VideoMatchMode, matchmode)
{
    EXPECT_EQ(v->setMatchMode(parm), 0);
    Playsleep(kPlaytime10S);
}

INSTANTIATE_TEST_SUITE_P(playbackFromMemory, VideoMatchMode,
         testing::Values(AV_VIDEO_MATCH_MODE_NONE, AV_VIDEO_MATCH_MODE_FULLSCREEN,
                AV_VIDEO_MATCH_MODE_LETTER_BOX, AV_VIDEO_MATCH_MODE_PAN_SCAN,
                AV_VIDEO_MATCH_MODE_COMBINED, AV_VIDEO_MATCH_MODE_WIDTHFULL,
                AV_VIDEO_MATCH_MODE_HEIGHFULL));

TEST_F(playbackFromMemory, videoDecoding)
{
    EXPECT_EQ(v->startDecoding(), 0);
    Playsleep(kPlaytime10S);

    EXPECT_EQ(v->pauseDecoding(), 0);
    Playsleep(kPlaytime10S);

    EXPECT_EQ(v->resumeDecoding(), 0);
    Playsleep(kPlaytime10S);

    EXPECT_EQ(v->resumeDecoding(), 0);
    Playsleep(kPlaytime10S);
}

TEST_F(playbackFromMemory, audioDecoding)
{
    int64_t t1, t2, t3, t4, t5, t6, t7, t8;

    EXPECT_EQ(a->startDecoding(), 0);
    EXPECT_EQ(av->getCurrentTime(&t1), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t2), 0);
    EXPECT_NE(t1, t2); //start decoding, expect time diff

    EXPECT_EQ(a->pauseDecoding(), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t3), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t4), 0);
    EXPECT_EQ(t3, t4); //pause decoding, expect time diff

    EXPECT_EQ(a->resumeDecoding(), 0);
    EXPECT_EQ(av->getCurrentTime(&t5), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t6), 0);
    EXPECT_NE(t5, t6); //resume decoding, expect time diff

    EXPECT_EQ(a->resumeDecoding(), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t7), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t8), 0);
    EXPECT_NE(t7, t8); //stop decoding, expect time diff
}


TEST_F(playbackFromMemory, adDecoding)
{
    int64_t t1, t2, t3, t4, t5, t6, t7, t8;

    EXPECT_EQ(v->startDecoding(), 0);
    EXPECT_EQ(av->getCurrentTime(&t1), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t2), 0);
    EXPECT_NE(t1, t2); //start decoding, expect time diff

    EXPECT_EQ(v->pauseDecoding(), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t3), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t4), 0);
    EXPECT_EQ(t3, t4); //pause decoding, expect time diff

    EXPECT_EQ(v->resumeDecoding(), 0);
    EXPECT_EQ(av->getCurrentTime(&t5), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t6), 0);
    EXPECT_NE(t5, t6); //resume decoding, expect time diff

    EXPECT_EQ(v->resumeDecoding(), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t7), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(av->getCurrentTime(&t8), 0);
    EXPECT_NE(t7, t8); //stop decoding, expect time diff
}

CLASS_PMEMORY_P(SetVol, int32_t);
TEST_P(SetVol, audioVol)
{
    EXPECT_EQ(a->setVolume(parm), 0);
    Playsleep(kPlaytime10S);
}
TEST_P(SetVol, adVol)
{
    EXPECT_EQ(ad->setVolume(parm), 0);
    Playsleep(kPlaytime10S);
}

INSTANTIATE_TEST_SUITE_P(playbackFromMemory, SetVol,
         testing::Values(0, 20, 40, 60, 80, 100));

CLASS_PMEMORY_P(SetStereoMode, am_tsplayer_audio_stereo_mode);
TEST_P(SetStereoMode, audioStereoMode)
{
    EXPECT_EQ(a->setStereoMode(parm), 0);
    Playsleep(kPlaytime10S);
}
TEST_P(SetStereoMode, adStereoMode)
{
    EXPECT_EQ(ad->setStereoMode(parm), 0);
    Playsleep(kPlaytime10S);
}

INSTANTIATE_TEST_SUITE_P(playbackFromMemory, SetStereoMode,
         testing::Values(AV_AUDIO_STEREO, AV_AUDIO_LEFT, AV_AUDIO_RIGHT,
                AV_AUDIO_MONO, AV_AUDIO_MULTICHANNEL));

class SetMute : public playbackFromMemory,
                public testing::WithParamInterface<std::pair<bool_t, bool_t>>
{
    public:
    virtual void SetUp() {
        playbackFromMemory::SetUp();
        amute = GetParam().first;
        dmute = GetParam().second;
    }
    protected:
        bool_t amute;
        bool_t dmute;
};

TEST_P(SetMute, audioMute)
{
    EXPECT_EQ(a->setMute(amute, dmute), 0);
    Playsleep(kPlaytime10S);
}
TEST_P(SetMute, adMute)
{
    EXPECT_EQ(ad->setMute(amute, dmute), 0);
    Playsleep(kPlaytime10S);
}
INSTANTIATE_TEST_SUITE_P(playbackFromMemory, SetMute,
         testing::Values(std::make_pair(false, false), std::make_pair(false, true),
                std::make_pair(true, false), std::make_pair(true, true)));

CLASS_PMEMORY_P(SetOutMode, am_tsplayer_audio_out_mode);
TEST_P(SetOutMode, audioOutMode)
{
    EXPECT_EQ(a->setOutMode(parm), 0);
    Playsleep(kPlaytime10S);
}
TEST_P(SetOutMode, adOutMode)
{
    EXPECT_EQ(ad->setOutMode(parm), 0);
    Playsleep(kPlaytime10S);
}

INSTANTIATE_TEST_SUITE_P(playbackFromMemory, SetOutMode,
         testing::Values(AV_AUDIO_OUT_PCM, AV_AUDIO_OUT_PASSTHROUGH,
                        AV_AUDIO_OUT_AUTO));

TEST_F(playbackFromMemory, adMix)
{
    EXPECT_EQ(ad->enableMix(), 0);
    Playsleep(kPlaytime10S);
    EXPECT_EQ(ad->disableMix(), 0);
    Playsleep(kPlaytime10S);
}

TEST_F(playbackFromMemory, getDefaultStreamInfoStat)
{
    Playsleep(kPlaytime10S);
    am_tsplayer_video_info vinfo;
    am_tsplayer_vdec_stat vstat;
    EXPECT_EQ(v->getInfo(&vinfo), 0);
    EXPECT_EQ(v->getStat(&vstat), 0);

    EXPECT_EQ(vinfo.width, kDefaultStreamVInfo.width);
    EXPECT_EQ(vinfo.height, kDefaultStreamVInfo.height);
    EXPECT_EQ(vinfo.framerate, kDefaultStreamVInfo.framerate);
    EXPECT_EQ(vinfo.bitrate, kDefaultStreamVInfo.bitrate);
    EXPECT_EQ(vinfo.ratio64, kDefaultStreamVInfo.ratio64);

    EXPECT_EQ(vinfo.width, vstat.frame_width);
    EXPECT_EQ(vinfo.height, vstat.frame_height);
    EXPECT_EQ(vinfo.framerate, vstat.frame_rate);
    EXPECT_EQ(vinfo.bitrate, vstat.bit_rate);
    EXPECT_EQ(vinfo.ratio64, vstat.ratio_control);

    am_tsplayer_audio_info ainfo;
    am_tsplayer_adec_stat astat;
    EXPECT_EQ(a->getInfo(&ainfo), 0);
    EXPECT_EQ(a->getStat(&astat), 0);

    EXPECT_EQ(ainfo.sample_rate, kDefaultStreamAInfo.sample_rate);
    EXPECT_EQ(ainfo.channels, kDefaultStreamAInfo.channels);
    EXPECT_EQ(ainfo.channel_mask, kDefaultStreamAInfo.channel_mask);
    EXPECT_EQ(ainfo.bitrate, kDefaultStreamAInfo.bitrate);
}

TEST_F(playbackFromMemory, getCurrentStreamInfoStat)
{
    Playsleep(kPlaytime10S);
    am_tsplayer_video_info vinfo;
    am_tsplayer_vdec_stat vstat;
    EXPECT_EQ(v->getInfo(&vinfo), 0);
    EXPECT_EQ(v->getStat(&vstat), 0);

    TLog("=========VINFO=========\n");
    TLog("width: %d\n", vinfo.width);
    TLog("height: %d\n", vinfo.height);
    TLog("framerate: %d\n", vinfo.framerate);
    TLog("bitrate: %d\n", vinfo.bitrate);
    TLog("ratio64: %llud\n", vinfo.ratio64);

    TLog("=========VSTAT=========\n");
    TLog("QOS.num: %d\n", vstat.qos.num);
    TLog("QOS.type: %d\n", vstat.qos.type);
    TLog("QOS.size: %d\n", vstat.qos.size);
    TLog("QOS.pts: %d\n", vstat.qos.pts);
    TLog("QOS.max_qp: %d\n", vstat.qos.max_qp);
    TLog("QOS.avg_qp: %d\n", vstat.qos.avg_qp);
    TLog("QOS.min_qp: %d\n", vstat.qos.min_qp);
    TLog("QOS.max_skip: %d\n", vstat.qos.max_skip);
    TLog("QOS.avg_skip: %d\n", vstat.qos.avg_skip);
    TLog("QOS.min_skip: %d\n", vstat.qos.min_skip);
    TLog("QOS.max_mv: %d\n", vstat.qos.max_mv);
    TLog("QOS.min_mv: %d\n", vstat.qos.min_mv);
    TLog("QOS.avg_mv: %d\n", vstat.qos.avg_mv);
    TLog("QOS.decode_buffer: %d\n", vstat.qos.decode_buffer);

    TLog("frame_width: %d\n", vstat.frame_width);
    TLog("frame_height: %d\n", vstat.frame_height);
    TLog("frame_rate: %d\n", vstat.frame_rate);
    TLog("bit_rate: %d\n", vstat.bit_rate);
    TLog("frame_dur: %d\n", vstat.frame_dur);
    TLog("frame_data: %d\n", vstat.frame_data);
    TLog("error_count: %d\n", vstat.error_count);
    TLog("status: %d\n", vstat.status);
    TLog("frame_count: %d\n", vstat.frame_count);
    TLog("error_frame_count: %d\n", vstat.error_frame_count);
    TLog("drop_frame_count: %d\n", vstat.drop_frame_count);
    TLog("total_data: %llud\n", vstat.total_data);
    TLog("samp_cnt: %d\n", vstat.samp_cnt);
    TLog("offset: %d\n", vstat.offset);
    TLog("ratio_control: %d\n", vstat.ratio_control);
    TLog("signal_type: %d\n", vstat.signal_type);
    TLog("pts: %d\n", vstat.pts);
    TLog("pts_us64: %llud\n", vstat.pts_us64);

    am_tsplayer_audio_info ainfo;
    am_tsplayer_adec_stat astat;
    if (mEnv.useAD) {
        EXPECT_EQ(ad->getInfo(&ainfo), 0);
        EXPECT_EQ(ad->getStat(&astat), 0);
    } else {
        EXPECT_EQ(a->getInfo(&ainfo), 0);
        EXPECT_EQ(a->getStat(&astat), 0);
    }
    TLog("=========AINFO=========\n");
    TLog("sample_rate: %d\n", ainfo.sample_rate);
    TLog("channels: %d\n", ainfo.channels);
    TLog("channel_mask: %d\n", ainfo.channel_mask);
    TLog("bitrate: %d\n", ainfo.bitrate);
    TLog("=========ASTAT=========\n");
    TLog("frame_count: %d\n", astat.frame_count);
    TLog("error_frame_count: %d\n", astat.error_frame_count);
    TLog("drop_frame_count: %d\n", astat.drop_frame_count);

}

class PlayerTest : public testing::Test {
protected:
    std::shared_ptr<AmTsPlayerSession>s;
    std::shared_ptr<AmTsPlayerSession::AVSync>av;
    std::shared_ptr<AmTsPlayerSession::Control>ctl;
    std::shared_ptr<AmTsPlayerSession::Video>v;
    std::shared_ptr<AmTsPlayerSession::Audio>a;
    std::shared_ptr<AmTsPlayerSession::AD>ad;
    std::shared_ptr<AmTsPlayerSession::Subtitle>sub;

    virtual void SetUp(){
        s = std::make_shared<AmTsPlayerSession>(
                    video_callback,
                    TS_MEMORY,
                    TS_PLAYER_MODE_NORMAL);
        av = s->mAVSync;
        ctl = s->mControl;
        v = s->mVideo;
        a = s->mAudio;
        ad = s->mAD;
        sub = s->mSubtitle;
    }
    virtual void TearDown(){
    }
};

TEST_F(PlayerTest, avsyncFunctionTest)
{
    int64_t time;
    EXPECT_EQ(av->getCurrentTime(&time), 0);

    am_tsplayer_avsync_mode mode;
    EXPECT_EQ(av->setSyncMode(TS_SYNC_AMASTER), 0);
    av->getSyncMode(&mode);
    EXPECT_EQ(mode, TS_SYNC_AMASTER);

    EXPECT_EQ(av->setSyncMode(TS_SYNC_VMASTER), 0);
    av->getSyncMode(&mode);
    EXPECT_EQ(mode, TS_SYNC_VMASTER);

    EXPECT_EQ(av->setSyncMode(TS_SYNC_PCRMASTER), 0);
    av->getSyncMode(&mode);
    EXPECT_EQ(mode, TS_SYNC_PCRMASTER);

    EXPECT_EQ(av->setSyncMode(TS_SYNC_NOSYNC), 0);
    av->getSyncMode(&mode);
    EXPECT_EQ(mode, TS_SYNC_NOSYNC);

    for (int i = 0; i < 255; i++) {
        EXPECT_EQ(av->setPcrPid(i), 0);
    }

    EXPECT_EQ(av->getDelayTime(&time), 0);
}

TEST_F(PlayerTest, controlFunctionTest)
{
    EXPECT_EQ(ctl->startFast(2.0), 0);
    EXPECT_EQ(ctl->startFast(0.5), 0);
    EXPECT_EQ(ctl->stopFast(), 0);

    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_NONE), 0);
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_PAUSE), 0);
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_PAUSE_NEXT), 0);
    EXPECT_EQ(ctl->setTrickMode(AV_VIDEO_TRICK_MODE_IONLY), 0);

    am_tsplayer_buffer_stat stat;
    EXPECT_EQ(ctl->getBufferStat(TS_STREAM_VIDEO, &stat), 0);
    EXPECT_EQ(ctl->getBufferStat(TS_STREAM_AUDIO, &stat), 0);
    EXPECT_EQ(ctl->getBufferStat(TS_STREAM_AD, &stat), 0);
    EXPECT_EQ(ctl->getBufferStat(TS_STREAM_SUB, &stat), 0);
}

TEST_F(PlayerTest, videoFunctionTest)
{
    EXPECT_EQ(v->setWindow(0, 0, 960, 540), 0);
    EXPECT_EQ(v->setWindow(0, 0, 1920, 1080), 0);
    EXPECT_NE(v->setWindow(0, 0, -1, -1), 0);
    EXPECT_NE(v->setWindow(0, 0, 4096, 4096), 0);

    EXPECT_EQ(v->setSurface(NULL), 0);
    EXPECT_EQ(v->setShow(), 0);
    EXPECT_EQ(v->setHide(), 0);

    EXPECT_EQ(v->setMatchMode(AV_VIDEO_MATCH_MODE_NONE), 0);
    EXPECT_EQ(v->setMatchMode(AV_VIDEO_MATCH_MODE_FULLSCREEN), 0);
    EXPECT_EQ(v->setMatchMode(AV_VIDEO_MATCH_MODE_LETTER_BOX), 0);
    EXPECT_EQ(v->setMatchMode(AV_VIDEO_MATCH_MODE_PAN_SCAN), 0);
    EXPECT_EQ(v->setMatchMode(AV_VIDEO_MATCH_MODE_COMBINED), 0);
    EXPECT_EQ(v->setMatchMode(AV_VIDEO_MATCH_MODE_WIDTHFULL), 0);
    EXPECT_EQ(v->setMatchMode(AV_VIDEO_MATCH_MODE_HEIGHFULL), 0);

    am_tsplayer_video_params vparm = {AV_VIDEO_CODEC_AUTO, 100};
    EXPECT_EQ(v->setParams(&vparm), 0);
    vparm.codectype = AV_VIDEO_CODEC_MPEG1;
    EXPECT_EQ(v->setParams(&vparm), 0);
    vparm.codectype = AV_VIDEO_CODEC_MPEG2;
    EXPECT_EQ(v->setParams(&vparm), 0);
    vparm.codectype = AV_VIDEO_CODEC_H264;
    EXPECT_EQ(v->setParams(&vparm), 0);
    vparm.codectype = AV_VIDEO_CODEC_H265;
    EXPECT_EQ(v->setParams(&vparm), 0);

    EXPECT_EQ(v->setBlackOut(true), 0);
    EXPECT_EQ(v->setBlackOut(false), 0);
    am_tsplayer_video_info vinfo;
    EXPECT_EQ(v->getInfo(&vinfo), 0);
    am_tsplayer_vdec_stat vstat;
    EXPECT_EQ(v->getStat(&vstat), 0);

    EXPECT_EQ(v->startDecoding(), 0);
    EXPECT_EQ(v->stopDecoding(), 0);
    EXPECT_EQ(v->pauseDecoding(), 0);
    EXPECT_EQ(v->resumeDecoding(), 0);
}

TEST_F(PlayerTest, audioFunctionTest)
{
    EXPECT_EQ(a->setVolume(100), 0);
    int32_t vol = 0;
    EXPECT_EQ(a->getVolume(&vol), 0);
    EXPECT_EQ(vol, 100);

    am_tsplayer_audio_stereo_mode smode;
    EXPECT_EQ(a->setStereoMode(AV_AUDIO_STEREO), 0);
    EXPECT_EQ(a->getStereoMode(&smode), 0);
    EXPECT_EQ(smode, AV_AUDIO_STEREO);

    EXPECT_EQ(a->setStereoMode(AV_AUDIO_LEFT), 0);
    EXPECT_EQ(a->getStereoMode(&smode), 0);
    EXPECT_EQ(smode, AV_AUDIO_LEFT);

    EXPECT_EQ(a->setStereoMode(AV_AUDIO_RIGHT), 0);
    EXPECT_EQ(a->getStereoMode(&smode), 0);
    EXPECT_EQ(smode, AV_AUDIO_RIGHT);

    EXPECT_EQ(a->setStereoMode(AV_AUDIO_MONO), 0);
    EXPECT_EQ(a->getStereoMode(&smode), 0);
    EXPECT_EQ(smode, AV_AUDIO_MONO);

    EXPECT_EQ(a->setStereoMode(AV_AUDIO_MULTICHANNEL), 0);
    EXPECT_EQ(a->getStereoMode(&smode), 0);
    EXPECT_EQ(smode, AV_AUDIO_MULTICHANNEL);

    bool_t amute, dmute;
    EXPECT_EQ(a->setMute(false, false), 0);
    EXPECT_EQ(a->getMute(&amute, &dmute), 0);
    EXPECT_EQ(amute, false);
    EXPECT_EQ(dmute, false);

    EXPECT_EQ(a->setMute(true, false), 0);
    EXPECT_EQ(a->getMute(&amute, &dmute), 0);
    EXPECT_EQ(amute, true);
    EXPECT_EQ(dmute, false);

    EXPECT_EQ(a->setMute(false, true), 0);
    EXPECT_EQ(a->getMute(&amute, &dmute), 0);
    EXPECT_EQ(amute, false);
    EXPECT_EQ(dmute, true);

    EXPECT_EQ(a->setMute(true, true), 0);
    EXPECT_EQ(a->getMute(&amute, &dmute), 0);
    EXPECT_EQ(amute, true);
    EXPECT_EQ(dmute, true);

    EXPECT_EQ(a->setMute(false, false), 0);
    EXPECT_EQ(a->getMute(&amute, &dmute), 0);
    EXPECT_EQ(amute, false);
    EXPECT_EQ(dmute, false);

    am_tsplayer_audio_params aparm = {AV_AUDIO_CODEC_AUTO, 100};
    EXPECT_EQ(a->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_MP2;
    EXPECT_EQ(a->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_MP3;
    EXPECT_EQ(a->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_AC3;
    EXPECT_EQ(a->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_EAC3;
    EXPECT_EQ(a->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_DTS;
    EXPECT_EQ(a->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_AAC;
    EXPECT_EQ(a->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_LATM;
    EXPECT_EQ(a->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_PCM;
    EXPECT_EQ(a->setParams(&aparm), 0);

    EXPECT_EQ(a->setOutMode(AV_AUDIO_OUT_PCM), 0);
    EXPECT_EQ(a->setOutMode(AV_AUDIO_OUT_PASSTHROUGH), 0);
    EXPECT_EQ(a->setOutMode(AV_AUDIO_OUT_AUTO), 0);

    am_tsplayer_audio_info ainfo;
    EXPECT_EQ(a->getInfo(&ainfo), 0);
    am_tsplayer_adec_stat astat;
    EXPECT_EQ(a->getStat(&astat), 0);

    EXPECT_EQ(a->startDecoding(), 0);
    EXPECT_EQ(a->stopDecoding(), 0);
    EXPECT_EQ(a->pauseDecoding(), 0);
    EXPECT_EQ(a->resumeDecoding(), 0);
}

TEST_F(PlayerTest, adFunctionTest)
{
    /*------ AD only ------*/
    int32_t mvol, svol;
    EXPECT_EQ(ad->setMixLevel(100, 100), 0);
    EXPECT_EQ(ad->getMixLevel(&mvol, &svol), 0);
    EXPECT_EQ(mvol, 100);
    EXPECT_EQ(svol, 100);

    EXPECT_EQ(ad->enableMix(), 0);
    EXPECT_EQ(ad->disableMix(), 0);
    /*------ AD only ------*/

    EXPECT_EQ(ad->setVolume(100), 0);
    int32_t vol = 0;
    EXPECT_EQ(ad->getVolume(&vol), 0);
    EXPECT_EQ(vol, 100);

    am_tsplayer_audio_stereo_mode smode;
    EXPECT_EQ(ad->setStereoMode(AV_AUDIO_STEREO), 0);
    EXPECT_EQ(ad->getStereoMode(&smode), 0);
    EXPECT_EQ(smode, AV_AUDIO_STEREO);

    EXPECT_EQ(ad->setStereoMode(AV_AUDIO_LEFT), 0);
    EXPECT_EQ(ad->getStereoMode(&smode), 0);
    EXPECT_EQ(smode, AV_AUDIO_LEFT);

    EXPECT_EQ(ad->setStereoMode(AV_AUDIO_RIGHT), 0);
    EXPECT_EQ(ad->getStereoMode(&smode), 0);
    EXPECT_EQ(smode, AV_AUDIO_RIGHT);

    EXPECT_EQ(ad->setStereoMode(AV_AUDIO_MONO), 0);
    EXPECT_EQ(ad->getStereoMode(&smode), 0);
    EXPECT_EQ(smode, AV_AUDIO_MONO);

    EXPECT_EQ(ad->setStereoMode(AV_AUDIO_MULTICHANNEL), 0);
    EXPECT_EQ(ad->getStereoMode(&smode), 0);
    EXPECT_EQ(smode, AV_AUDIO_MULTICHANNEL);

    bool_t amute, dmute;
    EXPECT_EQ(ad->setMute(false, false), 0);
    EXPECT_EQ(ad->getMute(&amute, &dmute), 0);
    EXPECT_EQ(amute, false);
    EXPECT_EQ(dmute, false);

    EXPECT_EQ(ad->setMute(true, false), 0);
    EXPECT_EQ(ad->getMute(&amute, &dmute), 0);
    EXPECT_EQ(amute, true);
    EXPECT_EQ(dmute, false);

    EXPECT_EQ(ad->setMute(false, true), 0);
    EXPECT_EQ(ad->getMute(&amute, &dmute), 0);
    EXPECT_EQ(amute, false);
    EXPECT_EQ(dmute, true);

    EXPECT_EQ(ad->setMute(true, true), 0);
    EXPECT_EQ(ad->getMute(&amute, &dmute), 0);
    EXPECT_EQ(amute, true);
    EXPECT_EQ(dmute, true);

    EXPECT_EQ(ad->setMute(false, false), 0);
    EXPECT_EQ(ad->getMute(&amute, &dmute), 0);
    EXPECT_EQ(amute, false);
    EXPECT_EQ(dmute, false);

    am_tsplayer_audio_params aparm = {AV_AUDIO_CODEC_AUTO, 100};
    EXPECT_EQ(ad->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_MP2;
    EXPECT_EQ(ad->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_MP3;
    EXPECT_EQ(ad->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_AC3;
    EXPECT_EQ(ad->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_EAC3;
    EXPECT_EQ(ad->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_DTS;
    EXPECT_EQ(ad->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_AAC;
    EXPECT_EQ(ad->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_LATM;
    EXPECT_EQ(ad->setParams(&aparm), 0);

    aparm.codectype = AV_AUDIO_CODEC_PCM;
    EXPECT_EQ(ad->setParams(&aparm), 0);

    EXPECT_EQ(ad->setOutMode(AV_AUDIO_OUT_PCM), 0);
    EXPECT_EQ(ad->setOutMode(AV_AUDIO_OUT_PASSTHROUGH), 0);
    EXPECT_EQ(ad->setOutMode(AV_AUDIO_OUT_AUTO), 0);

    am_tsplayer_audio_info ainfo;
    EXPECT_EQ(ad->getInfo(&ainfo), 0);
    am_tsplayer_adec_stat astat;
    EXPECT_EQ(ad->getStat(&astat), 0);

    EXPECT_EQ(ad->startDecoding(), 0);
    EXPECT_EQ(ad->stopDecoding(), 0);
    EXPECT_EQ(ad->pauseDecoding(), 0);
    EXPECT_EQ(ad->resumeDecoding(), 0);
}

TEST_F(PlayerTest, subtitleFunctionTest)
{
    /* start sub before setSubPid, fail*/
    EXPECT_NE(sub->startSub(), 0);

    for (int i = 0; i < 255; i++)
        EXPECT_EQ(sub->setSubPid(i), 0);
    EXPECT_EQ(sub->startSub(), 0);
    EXPECT_EQ(sub->stopSub(), 0);
}

const struct {
    const std::string tsname;
    am_tsplayer_video_codec vcodec;
    int32_t vpid;
    am_tsplayer_audio_codec acodec;
    int32_t apid;
} kStrTsToPlay[] = {
    { h264ts, AV_VIDEO_CODEC_H264, 0x100, AV_AUDIO_CODEC_AAC, 0x101},
    { h265ts, AV_VIDEO_CODEC_H265, 0x100, AV_AUDIO_CODEC_AAC, 0x101},
    { mpeg2ts, AV_VIDEO_CODEC_MPEG2, 0x100, AV_AUDIO_CODEC_AAC, 0x101},
};

const size_t kNumStrTsToPlay =
            sizeof(kStrTsToPlay) / sizeof(kStrTsToPlay[0]);


static bool CharToDigit(uint8_t ch, uint8_t* digit) {
  if (ch >= '0' && ch <= '9') {
    *digit = ch - '0';
  } else {
    ch = tolower(ch);
    if ((ch >= 'a') && (ch <= 'f')) {
      *digit = ch - 'a' + 10;
    } else {
      return false;
    }
  }
  return true;
}

uint32_t a2b_hex(const std::string& byte, uint8_t* out) {
    int count = byte.size();
    if (count == 0 || (count % 2) != 0 || !out)
        return -1;

    for (uint32_t i = 0; i < count / 2; ++i) {
        uint8_t msb = 0;  // most significant 4 bits
        uint8_t lsb = 0;  // least significant 4 bits
        if (!CharToDigit(byte[i * 2], &msb) ||
                !CharToDigit(byte[i * 2 + 1], &lsb))
            return -1;
        out[i] = (msb << 4) | lsb;
    }
    return 0;
}


class playbackFromShortHexstr : public testing::Test {
protected:
    std::shared_ptr<AmTsPlayerSession>s;
    uint8_t* mBuf;
    TestEnv mEnv;
    int32_t mSrcIdx = -1;

    virtual void SetUp(){
        s = std::make_shared<AmTsPlayerSession>(
                    video_callback,
                    gEnv.tsType,
                    TS_PLAYER_MODE_NORMAL);
        std::shared_ptr<AmTsPlayerSession::AVSync>&av = s->mAVSync;
        std::shared_ptr<AmTsPlayerSession::Control>&ctl = s->mControl;
        std::shared_ptr<AmTsPlayerSession::Video>&v = s->mVideo;
        std::shared_ptr<AmTsPlayerSession::Audio>&a = s->mAudio;
        memcpy(&mEnv, &gEnv, sizeof(gEnv));

        int fsize = kStrTsToPlay[mSrcIdx].tsname.size();
        mBuf = new uint8_t[fsize];
        a2b_hex(kStrTsToPlay[mSrcIdx].tsname, mBuf);

        av->setSyncMode(mEnv.avsyncMode);
        av->setPcrPid(mEnv.pcrPid);
        am_tsplayer_audio_params aparm;
        aparm.codectype = kStrTsToPlay[mSrcIdx].acodec;
        aparm.pid = kStrTsToPlay[mSrcIdx].apid;
        a->setParams(&aparm);
        a->startDecoding();
        am_tsplayer_video_params vparm;
        vparm.codectype = kStrTsToPlay[mSrcIdx].vcodec;
        vparm.pid = kStrTsToPlay[mSrcIdx].vpid;
        v->setParams(&vparm);
        v->startDecoding();
        ctl->setTrickMode(AV_VIDEO_TRICK_MODE_NONE);

        am_tsplayer_input_buffer ibuf = {TS_INPUT_BUFFER_TYPE_NORMAL, mBuf, fsize};
        int retry = 100;
        am_tsplayer_result res;
        do {
            res = s->writeData(&ibuf, kRwTimeout);
            if (res == AM_TSPLAYER_ERROR_RETRY) {
                printf("test error retry writeData res %d \n", res);
                usleep(50000);
            } else
                break;
        } while(res || retry-- > 0);
    }
    virtual void TearDown(){
        delete [](mBuf);
        TLog("TearDown\n");
    }
};

class playbackFromShortHexstrParam : public playbackFromShortHexstr,
                                     public ::testing::WithParamInterface<int32_t> {
    public:
        virtual void SetUp() {
        mSrcIdx = GetParam();
        playbackFromShortHexstr::SetUp();
    }
};

TEST_P(playbackFromShortHexstrParam, fiveSec)
{
    std::this_thread::sleep_for(std::chrono::seconds(5));
    uint32_t ver;
    EXPECT_EQ(s->getVersion(&ver), 0);
}

INSTANTIATE_TEST_SUITE_P(playbackFromShortHexstr, playbackFromShortHexstrParam,
        testing::Range(0, (int32_t)kNumStrTsToPlay));

TEST(TsPlayer, InitSharedptr)
{
    std::shared_ptr<AmTsPlayerSession>s;
    s = std::make_shared<AmTsPlayerSession>(
                video_callback,
                gEnv.tsType,
                TS_PLAYER_MODE_NORMAL);
}

TEST(TsPlayer, Init)
{
    AmTsPlayerSession* s = new AmTsPlayerSession(
                video_callback,
                gEnv.tsType,
                TS_PLAYER_MODE_NORMAL);
    delete s;
}

/*--------------------Demod--------------------*/

class playbackFromDemod : public testing::Test {
    public:
    std::shared_ptr<AmTsPlayerSession>s;
    std::shared_ptr<AmTsPlayerSession::AVSync>av;
    std::shared_ptr<AmTsPlayerSession::Control>ctl;
    std::shared_ptr<AmTsPlayerSession::Video>v;
    std::shared_ptr<AmTsPlayerSession::Audio>a;
    //std::shared_ptr<AmTsPlayerSession::AD>ad;
    //std::shared_ptr<AmTsPlayerSession::Subtitle>sub;
    TestEnv mEnv;
    virtual void SetUp(){
        s = std::make_shared<AmTsPlayerSession>(
                    video_callback,
                    TS_DEMOD,
                    TS_PLAYER_MODE_NORMAL);
        av = s->mAVSync;
        ctl = s->mControl;
        v = s->mVideo;
        a = s->mAudio;
        //ad = s->mAD;
        //sub = s->mSubtitle;

        memcpy(&mEnv, &gEnv, sizeof(gEnv));
        av->setSyncMode(gEnv.avsyncMode);
        av->setPcrPid(gEnv.pcrPid);
        am_tsplayer_audio_params aparm;
        aparm.codectype = gEnv.aCodec;
        aparm.pid = gEnv.aPid;
        a->setParams(&aparm);
        a->startDecoding();
        am_tsplayer_video_params vparm;
        vparm.codectype = gEnv.vCodec;
        vparm.pid = gEnv.vPid;
        v->setParams(&vparm);
        v->startDecoding();
        ctl->setTrickMode(AV_VIDEO_TRICK_MODE_NONE);
    }
    virtual void TearDown(){
    }
};

TEST_F(playbackFromDemod, play10S)
{
    usleep(kPlaytime10S);
}
#endif
////////////////////////////////////////////////////


template<typename T>
T envGet(const char* key, T def) {
    T val = def;
    if (!key)
        return val;
    char* p = getenv(key);
    if (p)
        val = static_cast<T>(atoi(p));
    return val;
}

//auto envGet(char* key, auto def) {
//    auto val = def;
//    if (!key)
//        return val;
//    char* p = getenv(key);
//    if (p)
//        val = decltype(def)(atoi(p));
//    return val;
//}

//#define envGet(type, key, def) \
//    static_cast<type>(key ? (getenv(key) ? atoi(getenv(key)) : def) : def)

void TestEnv::initFromEnv(void) {
    inputTsDir.assign(getenv("IDIR") ? getenv("IDIR") : "/data/");
    inputTsName.assign(getenv("INAME") ? getenv("INAME") : "1.ts");

    tsType = envGet("TSTYPE", TS_MEMORY);
    bufType = envGet("BUFTYPE", TS_INPUT_BUFFER_TYPE_NORMAL);
    avsyncMode = envGet("AVSYNCMODE", TS_SYNC_AMASTER);
    workMode = envGet("WORKMODE", TS_PLAYER_MODE_NORMAL);
    aStereoMode = envGet("STEREOMODE", AV_AUDIO_STEREO);
    aOutMode = envGet("AOUTMODE", AV_AUDIO_OUT_PCM);
    vTrickMode = envGet("TRICKMODE", AV_VIDEO_TRICK_MODE_NONE);
    vMatchMode = envGet("MATCHMODE", AV_VIDEO_MATCH_MODE_NONE);
    vCodec = envGet("VCODEC", AV_VIDEO_CODEC_H264);
    aCodec = envGet("ACODEC", AV_AUDIO_CODEC_AAC);
    dmxDevId = envGet("DMXID", 0);
    pcrPid = envGet("PCRPID", 0);
    vPid = envGet("VPID", 0x100);
    aPid = envGet("APID", 0x101);
    aMute = envGet("AMUTE", 0);
    dMute = envGet("DMUTE", 0);
    mVol = envGet("MVOL", 100);
    sVol = envGet("SVOL", 100);
    useAD = envGet("AD", 0);
}

static void usage(char **argv)
{
    printf("Usage: %s\n", argv[0]);
    printf("Version 0.1\n");
    printf("[options]:\n");
    printf("-i | --in           Ts file path\n");
    printf("-t | --tstype       demod:0, memory:1[default]\n");
    printf("-B | --buftype      nornal:0[default], secure:1\n");
    printf("-y | --avsync       amaster:0[default], vmaster:1, pcrmaster:2, nosync:3\n");
    printf("-w | --workmode     normal:0[default], cache only:1, decode only:2\n");
    printf("-s | --astereo      stereo:0[default], left:1, right:2, mono:3, multichannel:4\n");
    printf("-o | --aout         pcm:0[default], passthrough:1, auto:2\n");
    printf("-c | --vtrick       none:0[default], pause:1, pause next:2, Ionly:3\n");
    printf("-m | --vmatch       none:0[default], fullscreen:1, letter box:2,\n"
           "                    pan scan:3, combined:4, widthfull:5, heightfull:6\n");
    printf("-v | --vcodec       unknown:0[default], mpeg1:1, mpeg2:2, h264:3, h265:4, vp9:5\n");
    printf("-a | --acodec       unknown:0[default], mp2:1, mp3:2, ac3:3, eac3:4, dts:5, aac:6, latm:7, pcm:8\n");
    printf("-d | --dmxid        demux dev id, default 0\n");
    printf("-V | --vpid         video pid, default 0x100\n");
    printf("-A | --apid         audio pid, default 0x101\n");
    printf("-e | --amute        analog mute\n");
    printf("-f | --dmute        digital mute\n");
    printf("-M | --mvol         master vol, default 100\n");
    printf("-S | --svol         slave vol, default 100\n");
    printf("-D | --ad           AD, default false\n");
    printf("-y | --test         google test, default false\n");
    printf("-h | --help         print this usage\n");
}

void TestEnv::initFromOptions(int argc, char **argv) {
    int optionChar = 0;
    int optionIndex = 0;

#define IGNORE_ARGU(a) \
    {a, required_argument, NULL, 1}
#define IGNORE_NOARGU(a) \
    {a, no_argument, NULL, 1}

    const char *shortOptions = "i:t:b:y:w:s:o:c:m:v:a:d:V:A:M:S:D:efTh";
    struct option longOptions[] = {
        { "in",             required_argument,  NULL, 'i' },
        { "tstype",         required_argument,  NULL, 't' },
        { "buftype",        required_argument,  NULL, 'b' },
        { "avsync",         required_argument,  NULL, 'y' },
        { "workmode",       required_argument,  NULL, 'w' },
        { "audiostereo",    required_argument,  NULL, 's' },
        { "audioout",       required_argument,  NULL, 'o' },
        { "videotrick",     required_argument,  NULL, 'c' },
        { "videomatch",     required_argument,  NULL, 'm' },
        { "vcodec",         required_argument,  NULL, 'v' },
        { "acodec",         required_argument,  NULL, 'a' },
        { "dmxid",          required_argument,  NULL, 'd' },
        { "vpid",           required_argument,  NULL, 'V' },
        { "apid",           required_argument,  NULL, 'A' },
        { "amute",          no_argument,        NULL, 'e' },
        { "dmute",          no_argument,        NULL, 'f' },
        { "mvol",           required_argument,  NULL, 'M' },
        { "svol",           required_argument,  NULL, 'S' },
        { "ad",             no_argument,        NULL, 'D' },
        { "test",           no_argument,        NULL, 'T' },
        { "help",           no_argument,        NULL, 'h' },
        { "gtest",          required_argument,  NULL, 'g' },
        IGNORE_ARGU("gtest_filter"),
        IGNORE_ARGU("gtest_repeat"),
        IGNORE_ARGU("gtest_random_seed"),
        IGNORE_ARGU("gtest_color"),
        IGNORE_ARGU("gtest_print_time"),
        IGNORE_ARGU("gtest_output"),
        IGNORE_ARGU("gtest_catch_exceptions"),
        IGNORE_NOARGU("gtest_list_tests"),
        IGNORE_NOARGU("gtest_also_run_disabled_tests"),
        IGNORE_NOARGU("gtest_shuffle"),
        IGNORE_NOARGU("gtest_break_on_failure"),
        IGNORE_NOARGU("gtest_throw_on_failure"),
        { NULL,             0,                  NULL,  0  },
    };

    while ((optionChar = getopt_long(argc, argv, shortOptions,
                                    longOptions, &optionIndex)) != -1) {
        switch (optionChar) {
            case 'i':
                //inputTsName.replace(inputTsName.begin(),
                //        inputTsName.end(), (const char*)optarg);
                inputTsDir.clear();
                inputTsName.assign((const char*)optarg);
                break;
            case 't':
                tsType = static_cast<am_tsplayer_input_source_type>(atoi(optarg));
                break;
            case 'b':
                bufType = static_cast<am_tsplayer_input_buffer_type>(atoi(optarg));
                break;
            case 'y':
                avsyncMode = static_cast<am_tsplayer_avsync_mode>(atoi(optarg));
                break;
            case 'w':
                workMode = static_cast<am_tsplayer_work_mode>(atoi(optarg));
                break;
            case 's':
                aStereoMode = static_cast<am_tsplayer_audio_stereo_mode>(atoi(optarg));
                break;
            case 'o':
                aOutMode = static_cast<am_tsplayer_audio_out_mode>(atoi(optarg));
                break;
            case 'c':
                vTrickMode = static_cast<am_tsplayer_video_trick_mode>(atoi(optarg));
                break;
            case 'm':
                vMatchMode = static_cast<am_tsplayer_video_match_mode>(atoi(optarg));
                break;
            case 'v':
                vCodec = static_cast<am_tsplayer_video_codec>(atoi(optarg));
                break;
            case 'a':
                aCodec = static_cast<am_tsplayer_audio_codec>(atoi(optarg));
                break;
            case 'd':
                dmxDevId = atoi(optarg);
                break;
            case 'V':
                vPid = atoi(optarg);
                break;
            case 'A':
                aPid = atoi(optarg);
                break;
            case 'e':
                aMute = true;
                break;
            case 'f':
                dMute = true;
                break;
            case 'M':
                mVol = atoi(optarg);
                break;
            case 'S':
                sVol = atoi(optarg);
                break;
            case 'D':
                useAD = true;
                break;
            case 'h':
                usage(argv);
                exit(-1);
            case 'T':
            case 1:
                isGtest = true;
                break;
            default:
                break;
        }
    }

}

bool TestEnv::isGtestMode(void) {
    return isGtest;
}

static int set_osd_blank(int blank)
{
    const char *path1 = "/sys/class/graphics/fb0/blank";
    const char *path2 = "/sys/class/graphics/fb1/blank";
    const char *path3 = "/sys/class/graphics/fb0/osd_display_debug";
    int fd;
	char cmd[128] = {0};

	fd = open(path3,O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd >= 0)
	{
       sprintf(cmd,"%d",1);
	   write (fd,cmd,strlen(cmd));
	   close(fd);
	}
	fd = open(path1,O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd >= 0)
	{
       sprintf(cmd,"%d",blank);
	   write (fd,cmd,strlen(cmd));
	   close(fd);
	}
	fd = open(path2,O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd >= 0)
	{
       sprintf(cmd,"%d",blank);
	   write (fd,cmd,strlen(cmd));
	   close(fd);
	}
    return 0;
}

void signHandler(int iSignNo)
{
    printf("signHandler %d\n", iSignNo);
    set_osd_blank(0);
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
    printf("exit\n");
}

int main(int argc, char **argv)
{
    signal(SIGINT, signHandler);
    gEnv.initFromEnv();
    gEnv.initFromOptions(argc, argv);

    set_osd_blank(1);
    if (gEnv.isGtestMode()) {
        testing::InitGoogleTest(&argc, argv);
        int ret =  RUN_ALL_TESTS();
        set_osd_blank(0);
        return ret;
    }
    char* buf = new char[kRwSize];
	long fsize = 0;
    string ifile = gEnv.inputTsDir + gEnv.inputTsName;
    ifstream file(ifile.c_str(), ifstream::binary);
	if (gEnv.tsType)
	{
        printf("dir = %s, name = %s, ifile = %s, is_open %d\n",
            gEnv.inputTsDir.c_str(), gEnv.inputTsName.c_str(),
            ifile.c_str(), file.is_open());

        file.seekg(0, file.end);
        fsize = file.tellg();
        if (fsize <= 0) {
            printf("file size %ld return\n", fsize);
            return 0;
        }
        file.seekg(0, file.beg);

        printf("ifstream name %s size %ld\n",
                gEnv.inputTsName.c_str(), fsize);
	}
	else
	{
	    printf("playback from demod\n");
	}
    std::shared_ptr<AmTsPlayerSession>s;
    std::atomic_bool running;
    std::mutex eofLock;
    std::condition_variable eofCondition;

    s = std::make_shared<AmTsPlayerSession>(
                video_callback,
                gEnv.tsType,
                TS_PLAYER_MODE_NORMAL);

    std::shared_ptr<AmTsPlayerSession::AVSync>&av = s->mAVSync;
    std::shared_ptr<AmTsPlayerSession::Control>&ctl = s->mControl;
    std::shared_ptr<AmTsPlayerSession::Video>&v = s->mVideo;
    std::shared_ptr<AmTsPlayerSession::Audio>&a = s->mAudio;
    //std::shared_ptr<AmTsPlayerSession::AD>&ad = s->mAD;
    //std::shared_ptr<AmTsPlayerSession::Subtitle>&sub = s->mSubtitle;

    av->setSyncMode(gEnv.avsyncMode);
    av->setPcrPid(gEnv.pcrPid);
    am_tsplayer_audio_params aparm;
    aparm.codectype = gEnv.aCodec;
    aparm.pid = gEnv.aPid;
    a->setParams(&aparm);
    a->startDecoding();
    am_tsplayer_video_params vparm;
    vparm.codectype = gEnv.vCodec;
    vparm.pid = gEnv.vPid;
    v->setParams(&vparm);
    v->startDecoding();
    v->setShow();
    ctl->setTrickMode(AV_VIDEO_TRICK_MODE_NONE);

    running.store(static_cast<bool>(gEnv.tsType));
	printf("tsType %d\n",gEnv.tsType);
    std::thread rwThread([s, buf,/* fsize,*/ &file, &running, &eofLock, &eofCondition]() {
        am_tsplayer_input_buffer ibuf = {TS_INPUT_BUFFER_TYPE_NORMAL, (char*)buf, 0};
        long pos = 0;
        while (running)
        {
            if (file.eof()) {
                printf("file eof\n");
                std::unique_lock <std::mutex> l(eofLock);
                eofCondition.notify_all();
                break;
            }
            file.read(buf, (int)kRwSize);
            ibuf.buf_size = kRwSize;
            pos += kRwSize;

            int retry = 100;
            am_tsplayer_result res;
            do {
                res = s->writeData(&ibuf, kRwTimeout);
                if (res == AM_TSPLAYER_ERROR_RETRY) {
                    printf("test error retry\n");
                    usleep(50000);
                } else
                    break;
            } while(res || retry-- > 0);
        }
    });
    std::unique_lock <std::mutex> l(eofLock);
    eofCondition.wait_for(l, chrono::seconds(kPlayMaxSeconds));

    running.store(false);
    if (rwThread.joinable())
        rwThread.join();
    delete [](buf);
    if (file.is_open())
        file.close();

    set_osd_blank(0);
    printf("exit\n");
    return 0;
}


