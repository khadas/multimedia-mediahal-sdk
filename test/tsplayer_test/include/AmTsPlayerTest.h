#ifndef AMTSPLAYER_TEST_H
#define AMTSPLAYER_TEST_H

#include <gtest/gtest.h>
#include "AmTsPlayer.h"

using namespace std;

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

class TestEnv {
    public:
    std::string inputTsName;
    std::string inputTsDir;
    am_tsplayer_input_source_type tsType;
    am_tsplayer_input_buffer_type bufType;
    am_tsplayer_avsync_mode avsyncMode;
    am_tsplayer_work_mode workMode;
    am_tsplayer_audio_stereo_mode aStereoMode;
    am_tsplayer_audio_out_mode aOutMode;
    am_tsplayer_video_trick_mode vTrickMode;
    am_tsplayer_video_match_mode vMatchMode;
    am_tsplayer_video_codec vCodec;
    am_tsplayer_audio_codec aCodec;
    int32_t dmxDevId;
    int32_t pcrPid;
    int32_t vPid;
    int32_t aPid;
    bool aMute;
    bool dMute;
    int32_t mVol;
    int32_t sVol;
    bool useAD;
    bool isGtest;

    void initFromEnv(void);
    void initFromOptions(int argc, char **argv);
    bool isGtestMode(void);
};

extern TestEnv gEnv;
extern void tsleep(uint32_t us);

#endif // AMTSPLAYER_TEST_H
