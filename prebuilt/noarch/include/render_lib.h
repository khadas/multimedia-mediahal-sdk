#ifndef __RENDER_LIB_H__
#define __RENDER_LIB_H__
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include "render_common.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define RLIB_LOG_LEVEL_ERROR   0
#define RLIB_LOG_LEVEL_WARNING 1
#define RLIB_LOG_LEVEL_INFO    2
#define RLIB_LOG_LEVEL_DEBUG   3
#define RLIB_LOG_LEVEL_TRACE1  4
#define RLIB_LOG_LEVEL_TRACE2  5
#define RLIB_LOG_LEVEL_TRACE3  6

/*interval displaying video frame*/
typedef struct {
    int on; //if 1 means on, 0 means off
    int intervalMs; //step display frame interval, if this set to -1,only step one frame until next api invoked
} StepFrameInfo;

/*only hold video,audio output normal*/
typedef struct {
    int on;//if 1 means on, 0 means off
    int64_t pts; //the video pts of hold video,if requesting hold video immediately, please set pts to -1
} HoldVideoInfo;

/*render key*/
typedef enum {
    KEY_WINDOW_SIZE = 300, //set/get the video window size,value type is RenderRect
    KEY_FRAME_SIZE, //set/get frame size,value type is RenderFrameSize
    KEY_VIDEO_FORMAT, //set/get video pixel format, value type is int,detail see RenderVideoFormat enum
    KEY_VIDEO_FPS, //set/get video framerate, value type is int64_t, hight 32bit is numerator,low 32bit is denominator
    KEY_VIDEO_PIP, //set/get pip window flag, value type is int, 0:prime video,1:pip,this flag must set before render_connect
    KEY_FRAME_DROPPED,//get dropped video frames count,value type is int
    KEY_ZORDER, //set/get zorder of video plane,value type is int
    KEY_KEEP_LAST_FRAME, //set/get keep last frame when play end ,value type is int, 0 not keep, 1 keep
    KEY_HIDE_VIDEO, //set/get hide video,it effect immediatialy,value type is int, 0 not hide, 1 hide
    KEY_FORCE_ASPECT_RATIO, //set/get force pixel aspect ratio,value type is int, 1 is force,0 is not force
    KEY_SELECT_DISPLAY_OUTPUT,//set/get display output index,value type is int,0 is primary output and default value, 1 is extend display output
    KEY_IMMEDIATELY_OUTPUT, //set/get immediately output video frame to display, 0 is default value off, 1 is on
    KEY_STEP_DISPLAY_FRAME, //set/get step display frame,the value type is StepFrameInfo struct
    KEY_VIDEO_CROP, //set/get the video crop,value type is RenderWindowSize
    KEY_SHOW_FRIST_FRAME_NOSYNC, //set/get show first frame asap,please set it if before invoking render_display_frame
    KEY_HOLD_VIDEO, //set/get hold video,but audio is running,value type is HoldVideoInfo,detail info see HoldVideoInfo defined
    KEY_PIXEL_ASPECT_RATIO, //set/get video frame pixel aspect ratio, value type is double
    KEY_VIDEO_TRICK_MODE, //set/get video trick mode
    KEY_FRAME_BY_PASS_PLUGIN, //set/get video frame do not send to render plugin,frames will callback to user after got displaying monotime
    KEY_VIDEO_FRAME_RATE, //set video frame rate,value type is RenderFraction
    KEY_KEEP_LAST_FRAME_ON_FLUSH, //set/get keep last frame when seeking,value type is int, 0 not keep, 1 keep
    KEY_MEDIASYNC_INSTANCE_ID = 400, //set/get mediasync instance id, value type is int
    KEY_MEDIASYNC_PCR_PID, ///set/get mediasync pcr id ,value type is int
    KEY_MEDIASYNC_DEMUX_ID, //set/get mediasync demux id ,value type is int
    KEY_MEDIASYNC_SYNC_MODE, //set/get mediasync sync mode,value type is int, 0:vmaster,1:amaster,2:pcrmaster,3:videofreerun
    KEY_MEDIASYNC_TUNNEL_MODE, //set mediasync to use tunnel mode, 0:notunnelmode 1:tunnelmode
    KEY_MEDIASYNC_HAS_AUDIO, //set/get having audio,value type is int,0:not,1:has
    KEY_MEDIASYNC_VIDEOLATENCY, //set/get mediasync video latency
    KEY_MEDIASYNC_STARTTHRESHOLD, //set/get mediasync start threshold
    KEY_MEDIASYNC_VIDEOWORKMODE, //set/get mediasync video work mode
    KEY_MEDIASYNC_AUDIO_MUTE, //set/get audio mute
    KEY_MEDIASYNC_SOURCETYPE, //set/get media sync source type
    KEY_MEDIASYNC_VIDEOFRAME, //set/get
    KEY_MEDIASYNC_PLAYER_INSTANCE_ID,
    KEY_MEDIASYNC_PLAYBACK_RATE, //set/get playback rate,value type is float,0.5 is 0.5 rate, 1.0 is normal, 2.0 is 2x rate
    KEY_MEDIASYNC_VIDEO_SYNC_THRESHOLD, //set/get video free run threshold,value type is int,time is us
    KEY_MEDIASYNC_VIDEO_FREERUN, //set/get video freerun playback
    KEY_MEDIASYNC_SINGLE_DMX_NONTUNNELMODE, //set single dmx nontunnel mode
    //set/get video tunnel instance id when videotunnel plugin be selected,value type is int,this key must set before render_connect
    KEY_MEDIASYNC_MULTI_STREAM_MODE,
    KEY_MEDIASYNC_MULTI_STREAM_SYNC_ID,
    KEY_VIDEOTUNNEL_ID = 450,
    /*set render plugin to connect to compositor or disconnect from compositor, if set to 1
      and plugin is closed,render core will open plugin again,if set to 0 and plugin is opened,
      render core will close plugin. render core will do nothing if plugin is opened and set to 1.
      get render plugin state, 1:connected,0:disconnected*/
    KEY_PLUGIN_STATE = 600, //set render plugin connect to compositor,if plugin had connected,render plugin will do nothing,value type is NULL
    KEY_SET_USER_ID = 650, //set a special id to video render lib to identify user id,value type is int
} RenderKey;

/**
 * video render msg callback, user must register this callback to receive msg from video render
 *
 * @param userData the user data registered to video render lib
 * @param type  see enum RenderMsgType
 * @param msg it is difference according to type value. refer to RenderMsgType
 *
 */
typedef void (*onRenderMsgSend)(void *userData , RenderMsgType type, void *msg);
/**
 * video render lib gets values from user
 * @param key the value key,see enum _RenderKey
 * @param value the value return from user
 *
 * @return 0 success,-1 failure
 *
 */
typedef int (*onRenderGet)(void *userData, int key, void *value);

/**
 * a callback function that rend device call
 * this func to send msg to user,please not do
 * time-consuming action,please copy msg context,
 * render lib will free msg buffer when msg_callback called end
 * @param msg the message of sending
 * @return
 */
typedef struct {
    onRenderMsgSend doMsgSend;
    onRenderGet doGetValue;
} RenderCallback;

/**
 * set user log print function to render lib
 * @param callback log print callback function
 * @return void
*/
void render_set_log_callback(void (*callback)(int,const char*, va_list));

/**
 * set render lib log level, default level is RLIB_LOG_LEVEL_INFO
 * @param level render lib log level
 * @return void
*/
void render_set_log_level(int level);

/**
 * open a render lib,render lib will open a compositer  client
 *
 * @return a handle of render lib, null if failure
 */
void *render_open();

/**
 * registe callback to render lib, render lib will call
 * these callbacks to send msg to user or get some value from user
 * @param handle a handle of render lib that was opened before
 * @param callback  callback function
 * @return 0 success,-1 failure
 */
void render_set_callback(void *handle, void *userData, RenderCallback *callback);

/**
 * connect compositor client to compositor server
 * @param handle a handle of render lib that was opened before
 * @return 0 success,-1 failure
 */
int render_connect(void *handle);

/*************************************************/

/**
 * display a video frame, the buffer will be obtained by render lib
 * until render lib release it, so please allcating buffer from memory heap
 * @param handle a handle of render lib that was opened before
 * @param buffer a video buffer will be displayed
 * @return 0 success,-1 failure
 */
int render_display_frame(void *handle, RenderBuffer *buffer);

/**
 * set value to render lib
 * @param handle a handle of render lib that was opened before
 * @param key a key type,refer to RenderKey
 * @param value the value of key
 * @return 0 success,-1 failure
 */
int render_set_value(void *handle, RenderKey key, void *value);

/**
 * get value from render lib
 * @param handle a handle of render lib that was opened before
 * @param key a key type,refer to RenderKey
 * @param value the value of key
 * @return 0 success,-1 failure
 */
int render_get_value(void *handle, RenderKey key, void *value);

/**
 * flush render lib buffers
 * @param handle a handle of render lib that was opened before
 * @return 0 success,-1 failure
 */
int render_flush(void *handle);

/**
 * pause render lib, render lib will pause displaying video frames
 * @param handle a handle of render lib that was opened before
 * @return 0 success,-1 failure
 */
int render_pause(void *handle);

/**
 * pause render lib until video frame's pts reached to the special pts
 *
 * @param handle a handle of render lib that was opened
 * @param pts video frame pts, the pts unit is nano second
 * @return 0 success,-1 failure
 */
int render_pause_with_pts(void *handle, int64_t pts);

/**
 * resume render lib,render lib will resume displaying video frame
 * @param handle a handle of render lib that was opened before
 * @return 0 success,-1 failure
 */
int render_resume(void *handle);

/**
 * disconnect compositor client from compositor server
 * @param handle a handle of render lib that was opened before
 * @return 0 success,-1 failure
 */
int render_disconnect(void *handle);

/**
 * close render lib
 * @param handle a handle of render lib that was opened before
 * @return 0 success,-1 failure
 */
int render_close(void *handle);


/**********************tools func for render devices***************************/
/**
 * only allocating a RenderBuffer wrapper from render lib
 * @param handle a handle of render lib that was opened before
 * @param flag request allocating buffer flag, refer to RenderBufferFlag
 * @param rawBufferSize request allocating raw buffer size, if only allocate render buffer wrap, the bufferSize can 0
 * @return buffer handler or null if failure
 */
RenderBuffer *render_allocate_render_buffer_wrap(void *handle, int flag);

/**
 * free render buffer that allocated from render lib
 * @param handle a handle of render lib that was opened before
 * @return void
 */
void render_free_render_buffer_wrap(void *handle, RenderBuffer *buffer);

/**
 * get first audio pts from mediasync
 *
 * @param handle a handle of render lib that was opened before
 * @param pts the first rendered audio pts
 * @return int 0 success, -1 if failure
 */
int render_mediasync_get_first_audio_pts(void *handle, int64_t *pts);

/**
 * get current rendering audio pts from mediasync
 *
 * @param handle a handle of render lib that was opened before
 * @param pts the current rendering audio pts
 * @return int 0 success, -1 if failure
 */
int render_mediasync_get_current_audio_pts(void *handle, int64_t *pts);

/**
 * get current media time
 *
 * @param handle a handle of render lib that was opened before
 * @param mediaTimeType type of media
 * @param tunit type of time
 * @param mediaTime the current time
 * @return int 0 success, -1 if failure
 */
int render_mediasync_get_media_time_by_type(void *handle, int mediaTimeType, int tunit, int64_t *mediaTime);

/**
 * get playback rate from mediasync
 *
 * @param handle a handle of render lib that was opened before
 * @param scale the playback rate(output param)
 * @return int 0 success, -1 if failure
 */
int render_mediasync_get_playback_rate(void *handle, float *scale);

/**
 * queue pts that output from demux to mediasync for a/v sync
 *
 * @param handle a handle of render lib that was opened before
 * @param ptsUs the pts that output from demux, the unit is Us
 * @param size the frame size or 0 if unknown
 * @return int 0 success, -1 if failure
 */
int render_mediasync_queue_demux_pts(void *handle, int64_t ptsUs, uint32_t size);

/**
 * get first queuevideo pts from mediasync
 *
 * @param handle a handle of render lib that was opened before
 * @param pts the first queuevideo pts
 * @return int 0 success, -1 if failure
 */
int render_mediasync_get_first_queuevideo_pts(void *handle, int64_t *pts);

/**
 * get queuevideo pts from mediasync
 *
 * @param handle a handle of render lib that was opened before
 * @param pts the queuevideo pts
 * @return int 0 success, -1 if failure
 */
int render_mediasync_get_queuevideo_pts(void *handle, int64_t *pts);

/**
 * get first queueaudio pts from mediasync
 *
 * @param handle a handle of render lib that was opened before
 * @param pts the first queueaudio pts
 * @return int 0 success, -1 if failure
 */
int render_mediasync_get_first_queueaudio_pts(void *handle, int64_t *pts);

/**
 * get queueaudio pts from mediasync
 *
 * @param handle a handle of render lib that was opened before
 * @param pts the queueaudio pts
 * @return int 0 success, -1 if failure
 */
int render_mediasync_get_queueaudio_pts(void *handle, int64_t *pts);

#ifdef  __cplusplus
}
#endif
#endif /*__RENDER_LIB_H__*/