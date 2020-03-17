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

#define    RESMAN_IOC_MAGIC  'R'

#define    RESMAN_IOC_QUERY_RES          _IOR(RESMAN_IOC_MAGIC, 0x01, int)
#define    RESMAN_IOC_ACQUIRE_RES        _IOW(RESMAN_IOC_MAGIC, 0x02, int)
#define    RESMAN_IOC_RELEASE_RES        _IOR(RESMAN_IOC_MAGIC, 0x03, int)
#define    RESMAN_IOC_SETAPPINFO         _IOW(RESMAN_IOC_MAGIC, 0x04, int)

#define BASE_AVAILAB_RES               (0x0)
#define VFM_DEFAULT                    (BASE_AVAILAB_RES + 0)
#define AMVIDEO                        (BASE_AVAILAB_RES + 1)
#define PIPVIDEO                       (BASE_AVAILAB_RES + 2)
#define MAX_AVAILAB_RES                (PIPVIDEO + 1)

struct resman_para {
    int para_in;
    char para_str[32];
};

struct app_info {
    char appname[32];
    int  apptype;
};

typedef enum {
    TYPE_NONE     = -1,
    TYPE_OMX      = 0,
    TYPE_DVB,
    TYPE_HDMI_IN,
    TYPE_OTHER    = 10,
}APP_TYPE;

bool resman_support(void);
int resman_init(const char *appname, int type);
int resman_close(int handle);
int resman_setappinfo(int handle, struct app_info *appinfo);
int resman_acquire(int handle, int restype);
int resman_release(int handle, int restype);
int resman_query(int handle, struct resman_para *res_status);
bool resman_acquire_wait(int handle, int restype, const unsigned int time_out);//Timeout unit: milliseconds

#ifdef  __cplusplus
}
#endif

#endif /** _RESOURCE_MANAGE_H_ **/
