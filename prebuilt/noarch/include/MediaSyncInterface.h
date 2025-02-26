/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef MEDIA_SYNC_INTERFACE_H_
#define MEDIA_SYNC_INTERFACE_H_
#include <stdint.h>

typedef enum {
    MEDIA_SYNC_VMASTER = 0,
    MEDIA_SYNC_AMASTER = 1,
    MEDIA_SYNC_PCRMASTER = 2,
    MEDIA_SYNC_MODE_MAX = 255,
}sync_mode;

typedef enum {
    MEDIA_VIDEO = 0,
    MEDIA_AUDIO = 1,
    MEDIA_DMXPCR = 2,
    MEDIA_SUBTITLE = 3,
    MEDIA_COMMON = 4,
    MEDIA_TYPE_MAX = 255,
}sync_stream_type;

typedef enum {
    MEDIA_SYNC_NOSYNC = 0,
    MEDIA_SYNC_PAUSED = 1,
    MEDIA_SYNC_DISCONTINUE = 2,
    MEDIA_SYNC_STATUS_MAX = 255,
}sync_status;

typedef enum {
    AM_MEDIASYNC_OK  = 0,                      // OK
    AM_MEDIASYNC_ERROR_INVALID_PARAMS = -1,    // Parameters invalid
    AM_MEDIASYNC_ERROR_INVALID_OPERATION = -2, // Operation invalid
    AM_MEDIASYNC_ERROR_INVALID_OBJECT = -3,    // Object invalid
    AM_MEDIASYNC_ERROR_RETRY = -4,             // Retry
    AM_MEDIASYNC_ERROR_BUSY = -5,              // Device busy
    AM_MEDIASYNC_ERROR_END_OF_STREAM = -6,     // End of stream
    AM_MEDIASYNC_ERROR_IO            = -7,     // Io error
    AM_MEDIASYNC_ERROR_WOULD_BLOCK   = -8,     // Blocking error
    AM_MEDIASYNC_ERROR_MAX = -254
} mediasync_result;

typedef enum {
    MEDIASYNC_KEY_HASAUDIO = 0,
    MEDIASYNC_KEY_HASVIDEO,
    MEDIASYNC_KEY_VIDEOLATENCY,        //us
    MEDIASYNC_KEY_AUDIOFORMAT,
    MEDIASYNC_KEY_STARTTHRESHOLD,
    MEDIASYNC_KEY_ISOMXTUNNELMODE,
    MEDIASYNC_KEY_AUDIOCACHE,
    MEDIASYNC_KEY_VIDEOWORKMODE,
    MEDIASYNC_KEY_AUDIOMUTE,
    MEDIASYNC_KEY_SOURCETYPE,
    MEDIASYNC_KEY_ALSAREADY,
    MEDIASYNC_KEY_VSYNC_INTERVAL_MS,
    MEDIASYNC_KEY_VIDEOFRAME,
    MEDIASYNC_KEY_VIDEOTRICKMODE,
    MEDIASYNC_KEY_VIDEO_FREERUN,
    MEDIASYNC_KEY_AUDIO_FREERUN,
    MEDIASYNC_KEY_VIDEO_SYNC_THRESHOLD,  //us
    MEDIASYNC_KEY_AUDIO_SYNC_THRESHOLD,  //us
    MEDIASYNC_KEY_VIDEO_DURING_SLOW_SYNC,
    MEDIASYNC_KEY_VIDEO_FRAME_ADVANCE,
    MEDIASYNC_KEY_AUDIO_EQUIPMENT,
    MEDIASYNC_KEY_VIDEO_DRIVER_LAYER_SYNC,
    MEDIASYNC_KEY_SHOW_FIRST_FRAME_NOSYNC,
    MEDIASYNC_KEY_SLOWSYNC_ENABLE,
    MEDIASYNC_KEY_SLOWSYNC_PREROLL_RATE,
    MEDIASYNC_KEY_SLOWSYNC_AV_TOLERANCE,
    MEDIASYNC_KEY_VIDEO_HOLD,
    MEDIASYNC_KEY_MAX = 255,
} mediasync_parameter;

typedef enum {
    MEDIASYNC_UNIT_MS = 0,
    MEDIASYNC_UNIT_US,
    MEDIASYNC_UNIT_PTS,
    MEDIASYNC_UNIT_MAX,
} mediasync_time_unit;

typedef enum {
    MEDIASYNC_AUDIO_UNKNOWN = 0,
    MEDIASYNC_AUDIO_NORMAL_OUTPUT,
    MEDIASYNC_AUDIO_DROP_PCM,
    MEDIASYNC_AUDIO_INSERT,
    MEDIASYNC_AUDIO_HOLD,
    MEDIASYNC_AUDIO_MUTE,
    MEDIASYNC_AUDIO_RESAMPLE,
    MEDIASYNC_AUDIO_ADJUST_CLOCK,
    MEDIASYNC_AUDIO_EXIT,
} audio_policy;

typedef enum {
    MEDIASYNC_VIDEO_UNKNOWN = 0,
    MEDIASYNC_VIDEO_NORMAL_OUTPUT,
    MEDIASYNC_VIDEO_HOLD,
    MEDIASYNC_VIDEO_DROP,
    MEDIASYNC_VIDEO_EXIT,
} video_policy;

/*Video trick mode*/
typedef enum {
    VIDEO_TRICK_MODE_NONE = 0,          // Disable trick mode
    VIDEO_TRICK_MODE_PAUSE = 1,         // Pause the video decoder
    VIDEO_TRICK_MODE_PAUSE_NEXT = 2,    // Pause the video decoder when a new frame displayed
    VIDEO_TRICK_MODE_IONLY = 3          // Decoding and Out I frame only
} mediasync_video_trick_mode;

struct mediasync_audio_policy {
    audio_policy audiopolicy;
    int32_t  param1;
    int32_t  param2;
};

struct mediasync_video_policy {
    video_policy videopolicy;
    int64_t  param1;
    int32_t  param2;
};

struct mediasync_audio_format {
    int samplerate;
    int datawidth;
    int channels;
    int format;
};

typedef enum {
    MEDIA_VIDEO_TIME = 0,
    MEDIA_AUDIO_TIME = 1,
    MEDIA_DMXPCR_TIME = 2,
    MEDIA_STC_TIME = 3,
    MEDIA_TIME_TYPE_MAX = 255,
} media_time_type;

typedef struct audioinfo{
    int cacheSize;
    int cacheDuration;
}mediasync_audioinfo;

struct mediasync_audio_queue_info{
    int64_t apts;
    int size;
    int duration;
    mediasync_time_unit tunit;
    bool isworkingchannel;
    bool isneedupdate;
};

typedef struct frameinfo{
    int64_t framePts;
    int64_t frameSystemTime;
}mediasync_frameinfo;

extern void* MediaSync_create();

extern mediasync_result MediaSync_allocInstance(void* handle, int32_t DemuxId,
                                                              int32_t PcrPid,
                                                              int32_t *SyncInsId);

extern mediasync_result MediaSync_bindInstance(void* handle, uint32_t SyncInsId,
                                                             sync_stream_type streamtype);
extern mediasync_result MediaSync_bindStaticInstance(void* handle, uint32_t SyncInsId,
                                                             sync_stream_type streamtype);
extern mediasync_result MediaSync_setPlayerInsNumber(void* handle, int32_t number);
extern mediasync_result MediaSync_setSyncMode(void* handle, sync_mode mode);
extern mediasync_result MediaSync_getSyncMode(void* handle, sync_mode *mode);
extern mediasync_result MediaSync_setPause(void* handle, bool pause);
extern mediasync_result MediaSync_getPause(void* handle, bool *pause);
extern mediasync_result MediaSync_setStartingTimeMedia(void* handle, int64_t startingTimeMediaUs);
extern mediasync_result MediaSync_clearAnchor(void* handle);
extern mediasync_result MediaSync_updateAnchor(void* handle, int64_t anchorTimeMediaUs,
                                                                int64_t anchorTimeRealUs,
                                                                int64_t maxTimeMediaUs);
extern mediasync_result MediaSync_setPlaybackRate(void* handle, float rate);
extern mediasync_result MediaSync_getPlaybackRate(void* handle, float *rate);
extern mediasync_result MediaSync_getMediaTime(void* handle, int64_t realUs,
                                                                int64_t *outMediaUs,
                                                                bool allowPastMaxTime);
extern mediasync_result MediaSync_getRealTimeFor(void* handle, int64_t targetMediaUs, int64_t *outRealUs);
extern mediasync_result MediaSync_getRealTimeForNextVsync(void* handle, int64_t *outRealUs);
extern mediasync_result MediaSync_getTrackMediaTime(void* handle, int64_t *outMediaUs);
extern mediasync_result MediaSync_setUpdateTimeThreshold(void* handle, int64_t updateTimeThreshold);
extern mediasync_result MediaSync_getUpdateTimeThreshold(void* handle, int64_t* updateTimeThreshold);

extern mediasync_result mediasync_setParameter(void* handle, mediasync_parameter type, void* arg);
extern mediasync_result mediasync_getParameter(void* handle, mediasync_parameter type, void* arg);
extern mediasync_result MediaSync_queueAudioFrame(void* handle, struct mediasync_audio_queue_info* info);
extern mediasync_result MediaSync_queueVideoFrame(void* handle, int64_t vpts, int size, int duration, mediasync_time_unit tunit);
extern mediasync_result MediaSync_AudioProcess(void* handle, int64_t apts, int64_t cur_apts, mediasync_time_unit tunit, struct mediasync_audio_policy* asyncPolicy);
extern mediasync_result MediaSync_VideoProcess(void* handle, int64_t vpts, int64_t cur_vpts, mediasync_time_unit tunit, struct mediasync_video_policy* vsyncPolicy);
extern mediasync_result MediaSync_getFirstAudioFrameInfo(void* handle, mediasync_frameinfo* info);
extern mediasync_result MediaSync_getCurAudioFrameInfo(void* handle, mediasync_frameinfo* info);
extern mediasync_result MediaSync_GetMediaTimeByType(void* handle, media_time_type mediaTimeType,mediasync_time_unit tunit,int64_t* mediaTime);
extern mediasync_result MediaSync_getFirstVideoFrameInfo(void* handle, mediasync_frameinfo * arg);
extern mediasync_result MediaSync_GetFirstQueueVideoInfo(void* handle, mediasync_frameinfo *info);
extern mediasync_result MediaSync_GetQueueVideoInfo(void* handle, mediasync_frameinfo *info);
extern mediasync_result MediaSync_GetFirstQueueAudioInfo(void* handle, mediasync_frameinfo *info);
extern mediasync_result MediaSync_GetQueueAudioInfo(void* handle, mediasync_frameinfo *info);
extern mediasync_result MediaSync_RegisterCb(void* handle,int type,void* pfunc,void *cbHandle);
extern mediasync_result MediaSync_setPcrAndDmxId(void* handle, int32_t DemuxId, int32_t PcrPid);
extern mediasync_result MediaSync_reset(void* handle);
extern mediasync_result MediaSync_audioSwitch(void* handle, bool start, int64_t pts);
extern void MediaSync_destroy(void* handle);


#endif  // MEDIA_CLOCK_H_
