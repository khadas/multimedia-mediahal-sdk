#ifndef AMTSPLAYER_SESSION_H
#define AMTSPLAYER_SESSION_H

#include <memory>
#include "AmTsPlayer.h"

#define TLog(f, s...) \
do { \
        printf(f, ##s);\
} while(0)

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

typedef uint8_t         bool_t;

class AmTsPlayerSession
{
    public:
        AmTsPlayerSession(event_callback callback,
                        am_tsplayer_input_source_type type,
                        am_tsplayer_work_mode mode);
        virtual ~AmTsPlayerSession();

        am_tsplayer_result create(am_tsplayer_init_params Params);
        am_tsplayer_result getVersion(uint32_t *VersionM, uint32_t *VersionL);
        am_tsplayer_result getInstansNo(uint32_t *Numb);
        am_tsplayer_result register_cb(event_callback pfunc, void *param);
        am_tsplayer_result writeData(am_tsplayer_input_buffer *buf, uint64_t timeout_ms);
        am_tsplayer_result setWorkMode(am_tsplayer_work_mode mode);

        class AVSync {
            public:
                explicit AVSync(am_tsplayer_handle mSession);
                virtual ~AVSync();
                am_tsplayer_result getCurrentTime(
                            int64_t *time);
                am_tsplayer_result setSyncMode(
                            am_tsplayer_avsync_mode mode);
                am_tsplayer_result getSyncMode(
                            am_tsplayer_avsync_mode *mode);
                am_tsplayer_result setPcrPid(
                            uint32_t pid);
                am_tsplayer_result getDelayTime(
                            int64_t *time);
            private:
                am_tsplayer_handle mSession;
        };

        class Control {
            public:
                explicit Control(am_tsplayer_handle mSession);
                virtual ~Control();
                am_tsplayer_result startFast(
                            float scale);
                am_tsplayer_result stopFast();
                am_tsplayer_result setTrickMode(
                            am_tsplayer_video_trick_mode trickmode);
                am_tsplayer_result getBufferStat(
                            am_tsplayer_stream_type StrType,
                            am_tsplayer_buffer_stat *pBufStat);
            private:
                am_tsplayer_handle mSession;
        };

        class Video {
            public:
                explicit Video(am_tsplayer_handle mSession);
                virtual ~Video();
                am_tsplayer_result setWindow(
                            int32_t x,
                            int32_t y,
                            int32_t width,
                            int32_t height);
                am_tsplayer_result setSurface(
                            void* pSurface);
                am_tsplayer_result setShow();
                am_tsplayer_result setHide();
                am_tsplayer_result setMatchMode(
                            am_tsplayer_video_match_mode MathMod);
                am_tsplayer_result setParams(
                            am_tsplayer_video_params *pParams);
                am_tsplayer_result setBlackOut(
                            bool_t blackout);
                am_tsplayer_result getInfo(
                            am_tsplayer_video_info *pInfo);
                am_tsplayer_result getStat(
                            am_tsplayer_vdec_stat *pStat);
                am_tsplayer_result startDecoding();
                am_tsplayer_result pauseDecoding();
                am_tsplayer_result resumeDecoding();
                am_tsplayer_result stopDecoding();
            private:
                am_tsplayer_handle mSession;
        };

        class Audio {
            public:
                explicit Audio(am_tsplayer_handle mSession);
                virtual ~Audio();
                am_tsplayer_result setVolume(
                            int32_t volume);
                am_tsplayer_result getVolume(
                            int32_t *volume);
                am_tsplayer_result setStereoMode(
                            am_tsplayer_audio_stereo_mode Mode);
                am_tsplayer_result getStereoMode(
                            am_tsplayer_audio_stereo_mode *pMode);
                am_tsplayer_result setMute(
                            bool_t analog_mute,
                            bool_t digital_mute);
                am_tsplayer_result getMute(
                            bool_t *analog_unmute,
                            bool_t *digital_unmute);
                am_tsplayer_result setParams(
                            am_tsplayer_audio_params *pParams);
                am_tsplayer_result setOutMode(
                            am_tsplayer_audio_out_mode mode);
                am_tsplayer_result getInfo(
                            am_tsplayer_audio_info *pInfo);
                am_tsplayer_result getStat(
                            am_tsplayer_adec_stat *pStat);
                am_tsplayer_result startDecoding();
                am_tsplayer_result pauseDecoding();
                am_tsplayer_result resumeDecoding();
                am_tsplayer_result stopDecoding();
            private:
                am_tsplayer_handle mSession;
        };

        class AD {
            public:
                explicit AD(am_tsplayer_handle mSession);
                virtual ~AD();
                am_tsplayer_result setParams(
                            am_tsplayer_audio_params *pParams);
                am_tsplayer_result setMixLevel(
                            int32_t master_vol,
                            int32_t slave_vol);
                am_tsplayer_result getMixLevel(
                            int32_t *master_vol,
                            int32_t *slave_vol);
                am_tsplayer_result enableMix();
                am_tsplayer_result disableMix();
                am_tsplayer_result getInfo(
                            am_tsplayer_audio_info *pInfo);
                am_tsplayer_result getStat(
                            am_tsplayer_adec_stat *pStat);
                am_tsplayer_result setStereoMode(
                            am_tsplayer_audio_stereo_mode Mode);
                am_tsplayer_result getStereoMode(
                            am_tsplayer_audio_stereo_mode *pMode);
                am_tsplayer_result setVolume(
                            int32_t volume);
                am_tsplayer_result getVolume(
                            int32_t *volume);
                am_tsplayer_result setMute(
                            bool_t analog_mute,
                            bool_t digital_mute);
                am_tsplayer_result getMute(
                            bool_t* analog_unmute,
                            bool_t* digital_unmute);
                am_tsplayer_result setOutMode(
                            am_tsplayer_audio_out_mode mode);
                am_tsplayer_result startDecoding();
                am_tsplayer_result pauseDecoding();
                am_tsplayer_result resumeDecoding();
                am_tsplayer_result stopDecoding();
            private:
                am_tsplayer_handle mSession;
        };

        class Subtitle {
            public:
                explicit Subtitle(am_tsplayer_handle mSession);
                virtual ~Subtitle();
                am_tsplayer_result setSubPid(
                            int32_t pid);
                am_tsplayer_result startSub();
                am_tsplayer_result stopSub();
            private:
                am_tsplayer_handle mSession;
        };

        std::shared_ptr<AVSync> mAVSync;
        std::shared_ptr<Control> mControl;
        std::shared_ptr<Video> mVideo;
        std::shared_ptr<Audio> mAudio;
        std::shared_ptr<AD> mAD;
        std::shared_ptr<Subtitle> mSubtitle;

    private:
        am_tsplayer_handle mSession;
        int32_t mInstanceNo;
        event_callback mCallback;
        am_tsplayer_work_mode mWorkMode;
        am_tsplayer_input_source_type mTsType;
};

#endif // AMTSPLAYER_SESSION_H
