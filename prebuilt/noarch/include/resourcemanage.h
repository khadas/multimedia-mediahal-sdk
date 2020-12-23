/*
 * resourcemanage.h
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef _RESOURCE_MANAGE_H_
#define _RESOURCE_MANAGE_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <linux/ioctl.h>
#include <linux/types.h>
#include <sys/types.h>
#include <stdint.h>

#define    RESMAN_IOC_MAGIC  'R'

#define    RESMAN_IOC_QUERY_RES          _IOR(RESMAN_IOC_MAGIC, 0x01, int)
#define    RESMAN_IOC_ACQUIRE_RES        _IOW(RESMAN_IOC_MAGIC, 0x02, int)
#define    RESMAN_IOC_RELEASE_RES        _IOR(RESMAN_IOC_MAGIC, 0x03, int)
#define    RESMAN_IOC_SETAPPINFO         _IOW(RESMAN_IOC_MAGIC, 0x04, int)
#define    RESMAN_IOC_SUPPORT_RES        _IOR(RESMAN_IOC_MAGIC, 0x05, int)
#define    RESMAN_IOC_RELEASE_ALL        _IOR(RESMAN_IOC_MAGIC, 0x06, int)
#define    RESMAN_SUPPORT_PREEMPT        1

struct resman_para {
    __u32 k;
    union {
        struct {
            __u32 preempt;
            __u32 timeout;
            char arg[32];
        } acquire;
        struct {
            char name[32];
            __u32 type;
            __s32 value;
            __s32 avail;
        } query;
        struct {
            char name[32];
        } support;
    } v;
};

struct app_info {
    char appname[32];
    __u32  apptype;
};


enum RESMAN_ID {
    RESMAN_ID_VFM_DEFAULT,
    RESMAN_ID_AMVIDEO,
    RESMAN_ID_PIPVIDEO,
    RESMAN_ID_SEC_TVP,
    RESMAN_ID_TSPARSER,
    RESMAN_ID_CODEC_MM,
    RESMAN_ID_MAX,
};

enum RESMAN_TYPE {
    RESMAN_TYPE_COUNTER = 1,
    RESMAN_TYPE_TOGGLE,
    RESMAN_TYPE_TVP,
    RESMAN_TYPE_CODEC_MM
};

enum RESMAN_APP {
    RESMAN_APP_NONE = -1,
    RESMAN_APP_OMX  = 0,
    RESMAN_APP_DVB,
    RESMAN_APP_HDMI_IN,
    RESMAN_APP_SEC_TVP,
    RESMAN_APP_OTHER    = 10,
};

enum RESMAN_EVENT {
    RESMAN_EVENT_REGISTER       = 0x1000,
    RESMAN_EVENT_UNREGISTER,
    RESMAN_EVENT_PREEMPT,
    RESMAN_EVENT_STOP
};

bool resman_support(void);
int resman_init(const char *appname, int type);
int resman_close(int handle);
int resman_setappinfo(int handle, struct app_info *appinfo);
bool resman_acquire_para(int handle, int restype, const unsigned int time_out, int preempt, const char *arg);//Timeout unit: milliseconds
bool resman_acquire(int handle, int restype);
int resman_release(int handle, int restype);
int resman_release_all(int handle);
int resman_query(int handle, struct resman_para *res_status);
bool resman_acquire_wait(int handle, int restype, const unsigned int time_out);//Timeout unit: milliseconds
bool resman_resource_support(const char* resname);
int resman_register(int fd, void (* handler)(void *),  void *opaque);
void resman_unregister(int fd);
void resman_stop_thread();
int resman_estimate_size(int format, unsigned int width, unsigned int height);
#ifdef  __cplusplus
}
#endif

#endif /** _RESOURCE_MANAGE_H_ **/
