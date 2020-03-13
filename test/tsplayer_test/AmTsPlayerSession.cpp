#include "AmTsPlayerSession.h"

AmTsPlayerSession::AmTsPlayerSession(event_callback callback,
            am_tsplayer_input_source_type type,
            am_tsplayer_work_mode mode)
            : mCallback(callback),
            mWorkMode(mode),
            mTsType(type) {
    am_tsplayer_init_params parm = {mTsType, 0, 0};
    uint32_t versionM, versionL;
    uint32_t instanceno;

    create(parm);
    //CHECK_GE(mSession, 0);

    getVersion(&versionM, &versionL);
    getInstansNo(&instanceno);
    mInstanceNo = instanceno;

    setWorkMode(mWorkMode);
    register_cb(mCallback, NULL);

    mAVSync = std::make_shared<AVSync>(mSession);
    mControl = std::make_shared<Control>(mSession);
    mVideo = std::make_shared<Video>(mSession);
    mAudio = std::make_shared<Audio>(mSession);
    mAD = std::make_shared<AD>(mSession);
    mSubtitle = std::make_shared<Subtitle>(mSession);
}

AmTsPlayerSession::~AmTsPlayerSession()
{
    AmTsPlayer_release(mSession);
    TLog("~AmTsPlayerTestSession\n");
}

am_tsplayer_result AmTsPlayerSession::create(am_tsplayer_init_params Params) {
    return AmTsPlayer_create(Params, &mSession);
}

am_tsplayer_result AmTsPlayerSession::getVersion(uint32_t *VersionM, uint32_t *VersionL) {
    return AmTsPlayer_getVersion(VersionM, VersionL);
}

am_tsplayer_result AmTsPlayerSession::getInstansNo(uint32_t *Numb) {
    return AmTsPlayer_getInstansNo(mSession, Numb);
}

am_tsplayer_result AmTsPlayerSession::register_cb(event_callback pfunc, void *param) {
    return AmTsPlayer_registerCb(mSession, pfunc, param);
}

am_tsplayer_result AmTsPlayerSession::writeData(am_tsplayer_input_buffer *buf, uint64_t timeout_ms) {
    return AmTsPlayer_writeData(mSession, buf, timeout_ms);
}

am_tsplayer_result AmTsPlayerSession::setWorkMode(am_tsplayer_work_mode mode) {
    return AmTsPlayer_setWorkMode(mSession, mode);
}

/* AVSync */
AmTsPlayerSession::AVSync::AVSync(am_tsplayer_handle handle)
    : mSession(handle) {
    //TLog("AmTsPlayerSession::AVSync::AVSync\n");
}

AmTsPlayerSession::AVSync::~AVSync() {
    //TLog("~AVSync\n");
}

am_tsplayer_result AmTsPlayerSession::AVSync::getCurrentTime(
                int64_t *time) {
    return AmTsPlayer_getCurrentTime(mSession, time);
}

am_tsplayer_result AmTsPlayerSession::AVSync::setSyncMode(
                am_tsplayer_avsync_mode mode) {
    return AmTsPlayer_setSyncMode(mSession, mode);
}


am_tsplayer_result AmTsPlayerSession::AVSync::getSyncMode(
                am_tsplayer_avsync_mode *mode) {
    return AmTsPlayer_getSyncMode(mSession, mode);
}


am_tsplayer_result AmTsPlayerSession::AVSync::setPcrPid(
                uint32_t pid) {
    return AmTsPlayer_setPcrPid(mSession, pid);
}


am_tsplayer_result AmTsPlayerSession::AVSync::getDelayTime(
                int64_t *time) {
    return AmTsPlayer_getDelayTime(mSession, time);
}

/* Control */
AmTsPlayerSession::Control::Control(am_tsplayer_handle handle)
    : mSession(handle) {
    //TLog("AmTsPlayerSession::Control::Control\n");
}

AmTsPlayerSession::Control::~Control() {
    //TLog("~Control\n");
}

am_tsplayer_result AmTsPlayerSession::Control::startFast(
                float scale) {
    return AmTsPlayer_startFast(mSession, scale);
}

am_tsplayer_result AmTsPlayerSession::Control::stopFast() {
    return AmTsPlayer_stopFast(mSession);
}

am_tsplayer_result AmTsPlayerSession::Control::setTrickMode(
                am_tsplayer_video_trick_mode trickmode) {
    return AmTsPlayer_setTrickMode(mSession, trickmode);
}

am_tsplayer_result AmTsPlayerSession::Control::getBufferStat(
                am_tsplayer_stream_type StrType,
                am_tsplayer_buffer_stat *pBufStat) {
    return AmTsPlayer_getBufferStat(mSession, StrType, pBufStat);
}

/* Video */
AmTsPlayerSession::Video::Video(am_tsplayer_handle handle)
    : mSession(handle) {
    //TLog("AmTsPlayerSession::Video::Video\n");
}

AmTsPlayerSession::Video::~Video() {
    //TLog("~Video\n");
}

am_tsplayer_result AmTsPlayerSession::Video::setWindow(
            int32_t x,
            int32_t y,
            int32_t width,
            int32_t height) {
    return AmTsPlayer_setVideoWindow(mSession, x, y, width, height);
}

am_tsplayer_result AmTsPlayerSession::Video::setSurface(
            void* pSurface) {
    return AmTsPlayer_setSurface(mSession, pSurface);
}

am_tsplayer_result AmTsPlayerSession::Video::setShow() {
    return AmTsPlayer_showVideo(mSession);
}

am_tsplayer_result AmTsPlayerSession::Video::setHide() {
    return AmTsPlayer_hideVideo(mSession);
}

am_tsplayer_result AmTsPlayerSession::Video::setMatchMode(
            am_tsplayer_video_match_mode MathMod) {
    return AmTsPlayer_setVideoMatchMode(mSession, MathMod);
}

am_tsplayer_result AmTsPlayerSession::Video::setParams(
            am_tsplayer_video_params *pParams) {
    return AmTsPlayer_setVideoParams(mSession, pParams);
}

am_tsplayer_result AmTsPlayerSession::Video::setBlackOut(
            bool_t blackout) {
    return AmTsPlayer_setVideoBlackOut(mSession, blackout);
}

am_tsplayer_result AmTsPlayerSession::Video::getInfo(
            am_tsplayer_video_info *pInfo) {
    return AmTsPlayer_getVideoInfo(mSession, pInfo);
}

am_tsplayer_result AmTsPlayerSession::Video::getStat(
            am_tsplayer_vdec_stat *pStat) {
    return AmTsPlayer_getVideoStat(mSession, pStat);
}

am_tsplayer_result AmTsPlayerSession::Video::startDecoding() {
    return AmTsPlayer_startVideoDecoding(mSession);
}

am_tsplayer_result AmTsPlayerSession::Video::pauseDecoding() {
    return AmTsPlayer_pauseVideoDecoding(mSession);
}

am_tsplayer_result AmTsPlayerSession::Video::resumeDecoding() {
    return AmTsPlayer_resumeVideoDecoding(mSession);
}

am_tsplayer_result AmTsPlayerSession::Video::stopDecoding() {
    return AmTsPlayer_stopVideoDecoding(mSession);
}

/* Audio */
AmTsPlayerSession::Audio::Audio(am_tsplayer_handle handle)
    : mSession(handle) {
    //TLog("AmTsPlayerSession::Audio::Audio\n");
}

AmTsPlayerSession::Audio::~Audio() {
    //TLog("~Audio\n");
}

am_tsplayer_result AmTsPlayerSession::Audio::setVolume(
            int32_t volume) {
    return AmTsPlayer_setAudioVolume(mSession, volume);
}

am_tsplayer_result AmTsPlayerSession::Audio::getVolume(
            int32_t *volume) {
    return AmTsPlayer_getAudioVolume(mSession, volume);
}

am_tsplayer_result AmTsPlayerSession::Audio::setStereoMode(
            am_tsplayer_audio_stereo_mode Mode) {
    return AmTsPlayer_setAudioStereoMode(mSession, Mode);
}

am_tsplayer_result AmTsPlayerSession::Audio::getStereoMode(
            am_tsplayer_audio_stereo_mode *pMode) {
    return AmTsPlayer_getAudioStereoMode(mSession, pMode);
}

am_tsplayer_result AmTsPlayerSession::Audio::setMute(
            bool_t analog_mute,
            bool_t digital_mute) {
    return AmTsPlayer_setAudioMute(mSession, analog_mute, digital_mute);
}

am_tsplayer_result AmTsPlayerSession::Audio::getMute(
            bool_t *analog_unmute,
            bool_t *digital_unmute) {
    return AmTsPlayer_getAudioMute(mSession, analog_unmute, digital_unmute);
}

am_tsplayer_result AmTsPlayerSession::Audio::setParams(
            am_tsplayer_audio_params *pParams) {
    return AmTsPlayer_setAudioParams(mSession, pParams);
}

am_tsplayer_result AmTsPlayerSession::Audio::setOutMode(
            am_tsplayer_audio_out_mode mode) {
    return AmTsPlayer_setAudioOutMode(mSession, mode);
}

am_tsplayer_result AmTsPlayerSession::Audio::getInfo(
            am_tsplayer_audio_info *pInfo) {
    return AmTsPlayer_getAudioInfo(mSession, pInfo);
}

am_tsplayer_result AmTsPlayerSession::Audio::getStat(
            am_tsplayer_adec_stat *pStat) {
    return AmTsPlayer_getAudioStat(mSession, pStat);
}

am_tsplayer_result AmTsPlayerSession::Audio::startDecoding() {
    return AmTsPlayer_startAudioDecoding(mSession);
}

am_tsplayer_result AmTsPlayerSession::Audio::pauseDecoding() {
    return AmTsPlayer_pauseAudioDecoding(mSession);
}

am_tsplayer_result AmTsPlayerSession::Audio::resumeDecoding() {
    return AmTsPlayer_resumeAudioDecoding(mSession);
}

am_tsplayer_result AmTsPlayerSession::Audio::stopDecoding() {
    return AmTsPlayer_stopAudioDecoding(mSession);
}

/* AD */
AmTsPlayerSession::AD::AD(am_tsplayer_handle handle)
    : mSession(handle) {
    //TLog("AmTsPlayerSession::AD::AD\n");
}

AmTsPlayerSession::AD::~AD() {
    //TLog("~AD\n");
}

am_tsplayer_result AmTsPlayerSession::AD::setParams(
            am_tsplayer_audio_params *pParams) {
    return AmTsPlayer_setADParams(mSession, pParams);
}

am_tsplayer_result AmTsPlayerSession::AD::setMixLevel(
            int32_t master_vol,
            int32_t slave_vol) {
    return AmTsPlayer_setADMixLevel(mSession, master_vol, slave_vol);
}

am_tsplayer_result AmTsPlayerSession::AD::getMixLevel(
            int32_t *master_vol,
            int32_t *slave_vol) {
    return AmTsPlayer_getADMixLevel(mSession, master_vol, slave_vol);
}

am_tsplayer_result AmTsPlayerSession::AD::enableMix() {
    return AmTsPlayer_enableADMix(mSession);
}

am_tsplayer_result AmTsPlayerSession::AD::disableMix() {
    return AmTsPlayer_disableADMix(mSession);
}

am_tsplayer_result AmTsPlayerSession::AD::getInfo(
            am_tsplayer_audio_info *pInfo) {
    return AmTsPlayer_getADInfo(mSession, pInfo);
}

am_tsplayer_result AmTsPlayerSession::AD::getStat(
            am_tsplayer_adec_stat *pStat) {
    return AmTsPlayer_getADStat(mSession, pStat);
}

am_tsplayer_result AmTsPlayerSession::AD::setStereoMode(
            am_tsplayer_audio_stereo_mode Mode) {
    UNUSED(Mode);
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_setADStereoMode(mSession, Mode);
}

am_tsplayer_result AmTsPlayerSession::AD::getStereoMode(
            am_tsplayer_audio_stereo_mode *pMode) {
    UNUSED(pMode);
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_getADStereoMode(mSession, pMode);
}

am_tsplayer_result AmTsPlayerSession::AD::setVolume(
            int32_t volume) {
    UNUSED(volume);
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_setADVolume(mSession, volume);
}

am_tsplayer_result AmTsPlayerSession::AD::getVolume(
            int32_t *volume) {
    UNUSED(volume);
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_getADVolume(mSession, volume);
}

am_tsplayer_result AmTsPlayerSession::AD::setMute(
            bool_t analog_mute,
            bool_t digital_mute) {
    UNUSED(analog_mute);
    UNUSED(digital_mute);
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_setADMute(mSession, analog_mute, digital_mute);
}

am_tsplayer_result AmTsPlayerSession::AD::getMute(
            bool_t* analog_unmute,
            bool_t* digital_unmute) {
    UNUSED(analog_unmute);
    UNUSED(digital_unmute);
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_getADMute(mSession, analog_unmute, digital_unmute);
}

am_tsplayer_result AmTsPlayerSession::AD::setOutMode(
            am_tsplayer_audio_out_mode mode) {
    UNUSED(mode);
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_setADOutMode(mSession, mode);
}

am_tsplayer_result AmTsPlayerSession::AD::startDecoding() {
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_startADDecoding(mSession);
}

am_tsplayer_result AmTsPlayerSession::AD::pauseDecoding() {
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_pauseADDecoding(mSession);
}

am_tsplayer_result AmTsPlayerSession::AD::resumeDecoding() {
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_resumeADDecoding(mSession);
}

am_tsplayer_result AmTsPlayerSession::AD::stopDecoding() {
    return AM_TSPLAYER_OK;
    //return AmTsPlayer_stopADDecoding(mSession);
}

/* Subtitle */
AmTsPlayerSession::Subtitle::Subtitle(am_tsplayer_handle handle)
    : mSession(handle) {
    //TLog("AmTsPlayerSession::Subtitle::Subtitle\n");
}

AmTsPlayerSession::Subtitle::~Subtitle() {
    //TLog("~Subtitle\n");
}

am_tsplayer_result AmTsPlayerSession::Subtitle::setSubPid(
            int32_t pid) {
    return AmTsPlayer_setSubPid(mSession, pid);

}

am_tsplayer_result AmTsPlayerSession::Subtitle::startSub() {
    return AmTsPlayer_startSub(mSession);
}

am_tsplayer_result AmTsPlayerSession::Subtitle::stopSub() {
    return AmTsPlayer_stopSub(mSession);
}


