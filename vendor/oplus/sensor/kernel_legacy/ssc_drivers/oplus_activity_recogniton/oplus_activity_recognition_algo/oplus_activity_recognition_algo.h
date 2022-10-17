/******************************************************************
** Copyright (C), 2004-2020 OPPO Mobile Comm Corp., Ltd.
** OPLUS_FEATURE_ACTIVITY_RECOGNITION
** File: - oplus_activity_recognition_algo.h
** Description: Source file for oplus_activity_recognition sensor.
** Version: 1.0
** Date : 2020/07/01
**
** --------------------------- Revision History: ---------------------
* <version>            <date>             <author>                            <desc>
*******************************************************************/

#ifndef OPLUS_ACTIVITY_RECOGNITION_ALGO_H
#define OPLUS_ACTIVITY_RECOGNITION_ALGO_H

#define STAG "OPLUS_ACTIVITY_RECOGNITION - "

#ifdef CFG_MTK_SCP_OPLUS_ACTIVITY_RECOGNITION

#include "algoConfig.h"
#include <performance.h>
#include "contexthub_fw.h"
//#define OPLUS_ACTIVITY_RECOGNITION_LOG

#ifdef OPLUS_ACTIVITY_RECOGNITION_LOG
#define OPLUS_ACTIVITY_RECOGNITION_LOG_0(x...) osLog(LOG_ERROR, STAG x)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_1(x...) osLog(LOG_ERROR, STAG x)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_2(x...) osLog(LOG_ERROR, STAG x)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_3(x...) osLog(LOG_ERROR, STAG x)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_4(x...) osLog(LOG_ERROR, STAG x)
#else /*OPLUS_ACTIVITY_RECOGNITION_LOG*/
#define OPLUS_ACTIVITY_RECOGNITION_LOG_0(x...)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_1(x...)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_2(x...)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_3(x...)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_4(x...)
#endif /*OPLUS_ACTIVITY_RECOGNITION_LOG*/

#endif  /*CFG_MTK_SCP_OPLUS_ACTIVITY_RECOGNITION*/

#ifdef CFG_MSM_845_OPLUS_ACTIVITY_RECOGNITION

#include <math.h>
#include "sns_mem_util.h"
#include "sns_service_manager.h"
#include "sns_stream_service.h"
#include "sns_event_service.h"
#include "sns_rc.h"
#include "sns_diag_service.h"
#include "sns_printf.h"
#include "sns_memmgr.h"
#include "sns_printf_int.h"
#include "sns_island_util.h"
#include "sns_island.h"
#include "sns_assert.h"
#define OPLUS_ACTIVITY_RECOGNITION_LOG

#ifdef OPLUS_ACTIVITY_RECOGNITION_LOG
#define OPLUS_ACTIVITY_RECOGNITION_LOG_0(msg)                     SNS_SPRINTF(MED, sns_fw_printf, STAG msg)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_1(msg, p1)                 SNS_SPRINTF(MED, sns_fw_printf, STAG msg, p1)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_2(msg, p1, p2)             SNS_SPRINTF(MED, sns_fw_printf, STAG msg, p1, p2)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_3(msg, p1, p2, p3)         SNS_SPRINTF(MED, sns_fw_printf, STAG msg, p1, p2, p3)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_4(msg, p1, p2, p3, p4)     SNS_SPRINTF(MED, sns_fw_printf, STAG msg, p1, p2, p3, p4)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_5(msg, p1, p2, p3, p4, p5) SNS_SPRINTF(MED, sns_fw_printf, STAG msg, p1, p2, p3, p4, p5)
#else /*OPLUS_ACTIVITY_RECOGNITION_LOG*/
#define OPLUS_ACTIVITY_RECOGNITION_LOG_0(msg)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_1(msg, p1)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_2(msg, p1, p2)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_3(msg, p1, p2, p3)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_4(msg, p1, p2, p3, p4)
#endif /*OPLUS_ACTIVITY_RECOGNITION_LOG*/

#endif  /*CFG_MSM_845_OPLUS_ACTIVITY_RECOGNITION*/

#ifdef CFG_MSM_660_OPLUS_ACTIVITY_RECOGNITION

#include <math.h>
#include "sns_memmgr.h"
#include "sns_em.h"
#include "fixed_point.h"

//#define OPLUS_ACTIVITY_RECOGNITION_LOG

#ifdef OPLUS_ACTIVITY_RECOGNITION_LOG
#define OPLUS_ACTIVITY_RECOGNITION_LOG_0(msg)                    UMSG(MSG_SSID_SNS, DBG_ERROR_PRIO, STAG msg)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_1(msg, p1)                UMSG_1(MSG_SSID_SNS, DBG_ERROR_PRIO, STAG msg, p1)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_2(msg, p1, p2)            UMSG_2(MSG_SSID_SNS, DBG_ERROR_PRIO, STAG msg, p1, p2)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_3(msg, p1, p2, p3)        UMSG_3(MSG_SSID_SNS, DBG_ERROR_PRIO, STAG msg, p1, p2, p3)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_4(msg, p1, p2, p3, p4)    UMSG_4(MSG_SSID_SNS, DBG_ERROR_PRIO, STAG msg, p1, p2, p3, p4)
#else /*OPLUS_ACTIVITY_RECOGNITION_LOG*/
#define OPLUS_ACTIVITY_RECOGNITION_LOG_0(msg)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_1(msg, p1)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_2(msg, p1, p2)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_3(msg, p1, p2, p3)
#define OPLUS_ACTIVITY_RECOGNITION_LOG_4(msg, p1, p2, p3, p4)
#endif /*OPLUS_ACTIVITY_RECOGNITION_LOG*/

#endif  /*CFG_MSM_660_OPLUS_ACTIVITY_RECOGNITION*/

#define BUFFER_SIZE         20
#define MAX_MOTION_BUFF     20

typedef enum
{
    AP_SUSPEND,
    AP_AWAKE,
} AP_STATUS;

typedef enum
{
    REMOTE_PROC_TYPE,
    MOTION_RECOGNITION_TYPE,
    MAX_TYPE,
} SENSOR_TYPE;

typedef struct
{
    float x_value;
    float y_value;
    float z_value;

#ifdef CFG_MSM_845_OPLUS_ACTIVITY_RECOGNITION
    uint64_t timestamp;
#else
    uint32_t timestamp;
#endif

    SENSOR_TYPE type;
} oplus_activity_recognition_sensor_data;

struct oplus_activity_recognition_sensor_operation
{
    void (*sensor_change_state)(int sensor_type, bool on);
    void (*report)(int motioncount, int ith, int incar, int activity, uint64_t delta_time);
    uint64_t (*get_delta_time_ms)(uint64_t timestamp);

    uint64_t (*get_current_time_tick)(void);
};

struct pre_motion_info_st
{
    uint8_t incar_state;
    uint8_t activity_mode;
    uint64_t timestamp;
};

typedef struct
{
    uint8_t motion_count;
    uint8_t current_ap_state;
    struct pre_motion_info_st pre_motion_info[MAX_MOTION_BUFF];
    struct oplus_activity_recognition_sensor_operation *sensor_ops;
} oplus_activity_recognition_state;

void oplus_activity_recognition_algo_register(oplus_activity_recognition_state **state);

void oplus_activity_recognition_Reset(void);

void oplus_activity_recognition_close(void);

void oplus_activity_recognition_check_sensor_data(oplus_activity_recognition_sensor_data *input);

#endif //OPLUS_ACTIVITY_RECOGNITION_ALGO_H
