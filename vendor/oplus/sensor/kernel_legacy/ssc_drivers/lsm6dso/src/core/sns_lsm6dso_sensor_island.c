/**
 * @file sns_lsm6dso_sensor.c
 *
 * Common implementation for LSM6DSO Sensors.
 *
 * Copyright (c) 2020, STMicroelectronics.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *     2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     3. Neither the name of the STMicroelectronics nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/

#include <string.h>
#include "sns_attribute_util.h"
#include "sns_event_service.h"
#include "sns_diag_service.h"
#include "sns_island_service.h"
#include "sns_lsm6dso_sensor.h"
#include "sns_math_util.h"
#include "sns_mem_util.h"
#include "sns_sensor_instance.h"
#include "sns_service_manager.h"
#include "sns_sensor_util.h"
#include "sns_service.h"
#include "sns_stream_service.h"
#include "sns_types.h"

#include "pb_encode.h"
#include "pb_decode.h"
#include "sns_motion_detect.pb.h"
#include "sns_pb_util.h"
#include "sns_printf.h"
#include "sns_registry.pb.h"
#include "sns_std.pb.h"
#include "sns_std_sensor.pb.h"
#include "sns_std_event_gated_sensor.pb.h"
#include "sns_suid.pb.h"
#include "sns_timer.pb.h"
#include "sns_cal.pb.h"

/** Forward Declaration of Accel Sensor API */
sns_sensor_api lsm6dso_accel_sensor_api;

/** Forward Declaration of Gyro Sensor API */
sns_sensor_api lsm6dso_gyro_sensor_api;

/** Forward Declaration of motion detect Sensor API */
sns_sensor_api lsm6dso_motion_detect_sensor_api;

/** Forward Declaration of Sensor temp Sensor API */
sns_sensor_api lsm6dso_sensor_temp_sensor_api;

//define non-dependent sensors at the begining
//then dependent sensors ex: MA depends on accel
//so MA shoule be defined below the accel
//Not following the order fails sensor configuration
const lsm6dso_sensors lsm6dso_supported_sensors[ MAX_SUPPORTED_SENSORS ] = {
  { LSM6DSO_ACCEL,                     ALIGN_8(sizeof(lsm6dso_state)) + sizeof(lsm6dso_shared_state),
    &(sns_sensor_uid)ACCEL_SUID_0,
    &lsm6dso_accel_sensor_api,
    &lsm6dso_sensor_instance_api},
  { LSM6DSO_GYRO,                      sizeof(lsm6dso_state),
    &(sns_sensor_uid)GYRO_SUID_0,
    &lsm6dso_gyro_sensor_api,
    &lsm6dso_sensor_instance_api},
  { LSM6DSO_MOTION_DETECT,             sizeof(lsm6dso_state),
    &(sns_sensor_uid)MOTION_DETECT_SUID_0,
    &lsm6dso_motion_detect_sensor_api,
    &lsm6dso_sensor_instance_api},
  { LSM6DSO_SENSOR_TEMP,               sizeof(lsm6dso_state),
    &(sns_sensor_uid)SENSOR_TEMPERATURE_SUID_0,
    &lsm6dso_sensor_temp_sensor_api,
    &lsm6dso_sensor_instance_api}
};
//static char reg_dependency[][MAX_DEP_LENGTH] =  {"registry" };
//static char add_dependency[][MAX_DEP_LENGTH]= {""};

static void lsm6dso_handle_flush_request(
  sns_sensor           *this,
  sns_sensor_instance  *instance,
  lsm6dso_shared_state *shared_state);

static void  lsm6dso_send_flush_config(
  sns_sensor          *const this,
  sns_sensor_instance *instance,
  lsm6dso_sensor_type sensor);


#if !LSM6DSO_ISLAND_DISABLED
void lsm6dso_exit_island(sns_sensor *const this)
{
  sns_service_manager *smgr = this->cb->get_service_manager(this);
  sns_island_service  *island_svc  =
    (sns_island_service *)smgr->get_service(smgr, SNS_ISLAND_SERVICE);
  island_svc->api->sensor_island_exit(island_svc, this);
}
#else
void lsm6dso_exit_island(sns_sensor *const this)
{
  UNUSED_VAR(this);
}
#endif

#if !LSM6DSO_POWERRAIL_DISABLED
static void lsm6dso_start_power_rail_timer(
  sns_sensor                       *const this,
  sns_time                         timeout_ticks,
  lsm6dso_power_rail_pending_state pwr_rail_pend_state,
  lsm6dso_shared_state             *shared_state)
{
  if(NULL == shared_state->timer_stream)
  {
    sns_service_manager *service_mgr = this->cb->get_service_manager(this);
    sns_stream_service *stream_svc = (sns_stream_service*)
      service_mgr->get_service(service_mgr, SNS_STREAM_SERVICE);
    DBG_PRINTF_EX(LOW, this, "power_rail_timer: master sensor creating stream");
    stream_svc->api->create_sensor_stream(stream_svc, lsm6dso_get_master_sensor(this),
                                          shared_state->inst_cfg.timer_suid,
                                          &shared_state->timer_stream);
  }

  if(NULL != shared_state->timer_stream)
  {
    sns_timer_sensor_config req_payload = sns_timer_sensor_config_init_default;
    size_t req_len;
    uint8_t buffer[20];
    sns_memset(buffer, 0, sizeof(buffer));
    req_payload.is_periodic = false;
    req_payload.start_time = sns_get_system_time();
    req_payload.timeout_period = timeout_ticks;

    req_len = pb_encode_request(buffer, sizeof(buffer), &req_payload,
                                sns_timer_sensor_config_fields, NULL);
    if(req_len > 0)
    {
      sns_request timer_req =
        {  .message_id = SNS_TIMER_MSGID_SNS_TIMER_SENSOR_CONFIG,
           .request = buffer, .request_len = req_len};
      sns_rc rc = shared_state->timer_stream->api->send_request(shared_state->timer_stream,
                                                                &timer_req);
      if(SNS_RC_SUCCESS == rc)
      {
        shared_state->power_rail_pend_state = pwr_rail_pend_state;
      }
    }
  }
}

static void lsm6dso_turn_rails_off(sns_sensor *this, lsm6dso_shared_state *shared_state)
{
  if((SNS_RAIL_OFF != shared_state->rail_config.rail_vote) &&
     (LSM6DSO_POWER_RAIL_PENDING_NONE == shared_state->power_rail_pend_state))
  {
    sns_time timeout = sns_convert_ns_to_ticks(LSM6DSO_POWER_RAIL_OFF_TIMEOUT_NS);
    lsm6dso_start_power_rail_timer(this, timeout, LSM6DSO_POWER_RAIL_PENDING_OFF, shared_state);
  }
}

static void lsm6dso_check_pending_flush_requests(
  sns_sensor           *this,
  sns_sensor_instance  *instance)
{
  sns_sensor *lib_sensor;
  for(lib_sensor = this->cb->get_library_sensor(this, true);
      NULL != lib_sensor;
      lib_sensor = this->cb->get_library_sensor(this, false))
  {
    lsm6dso_state *state = (lsm6dso_state*)lib_sensor->state->state;
    if(state->flush_req)
    {
      state->flush_req = false;
      DBG_PRINTF(MED, this, "pending_flush_requests: sensor=%u", state->sensor);
      lsm6dso_send_fifo_flush_done(instance, state->sensor, FLUSH_DONE_CONFIGURING);
    }
  }
}


static sns_rc lsm6dso_process_timer_events(sns_sensor *const this)
{
  sns_rc rv = SNS_RC_SUCCESS;
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  UNUSED_VAR(state);
  lsm6dso_shared_state *shared_state = lsm6dso_get_shared_state(this);
  sns_data_stream *stream = shared_state->timer_stream;

  if(NULL == stream || 0 == stream->api->get_input_cnt(stream))
  {
    return rv;
  }

  for(sns_sensor_event *event = stream->api->peek_input(stream);
      NULL != event;
      event = stream->api->get_next_input(stream))
  {
    sns_timer_sensor_event timer_event;
    pb_istream_t pbstream;

    if(SNS_TIMER_MSGID_SNS_TIMER_SENSOR_EVENT != event->message_id)
    {
      continue; /* not interested in other events */
    }

    pbstream = pb_istream_from_buffer((pb_byte_t*)event->event, event->event_len);
    if(!pb_decode(&pbstream, sns_timer_sensor_event_fields, &timer_event))
    {
      SNS_PRINTF(ERROR, this, "pb_dec err");
      continue;
    }

    DBG_PRINTF_EX(HIGH, this, "Timer fired: sensor=%u pend_state=%u current=%u",
               state->sensor, shared_state->power_rail_pend_state,
               shared_state->rail_config.rail_vote);

    if(shared_state->power_rail_pend_state == LSM6DSO_POWER_RAIL_PENDING_INIT)
    {
      lsm6dso_com_port_info *com_port = &shared_state->inst_cfg.com_port_info;
      shared_state->power_rail_pend_state = LSM6DSO_POWER_RAIL_PENDING_NONE;
      /** Initial HW discovery is OK to run in normal mode. */
      lsm6dso_exit_island(this);

      /**-----------------Try to enter I3C mode-------------------------*/
      rv = lsm6dso_enter_i3c_mode(NULL, com_port, shared_state->scp_service);
      DBG_PRINTF_EX(LOW, this, "enter_i3c_mode returns %d", rv);

      rv = lsm6dso_discover_hw(this);
    }
    else if(shared_state->power_rail_pend_state == LSM6DSO_POWER_RAIL_PENDING_SET_CLIENT_REQ)
    {
      sns_sensor_instance *instance = sns_sensor_util_get_shared_instance(this);
      shared_state->power_rail_pend_state = LSM6DSO_POWER_RAIL_PENDING_NONE;
      if(NULL != instance)
      {
        lsm6dso_set_client_config(this, instance, shared_state);
        lsm6dso_check_pending_flush_requests(this, instance);
      } else {
        DBG_PRINTF(LOW, this, "instance no longer available");
        lsm6dso_turn_rails_off(this, shared_state);
      }
    }
    else if(shared_state->power_rail_pend_state == LSM6DSO_POWER_RAIL_PENDING_OFF)
    {
      DBG_PRINTF_EX(HIGH, this, "Turning off power rail");
      shared_state->power_rail_pend_state = LSM6DSO_POWER_RAIL_PENDING_NONE;
      lsm6dso_update_rail_vote(this, shared_state, SNS_RAIL_OFF, NULL);
    }
  }
  if(shared_state->power_rail_pend_state == LSM6DSO_POWER_RAIL_PENDING_NONE)
  {
    sns_sensor_util_remove_sensor_stream(this, &shared_state->timer_stream);
  }
  return rv;
}
#endif

/* See sns_sensor::get_sensor_uid */
sns_sensor_uid const* lsm6dso_get_sensor_uid(sns_sensor const *const this)
{
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  return &state->my_suid;
}

/** See sns_lsm6dso_sensor.h*/
sns_rc lsm6dso_sensor_notify_event(sns_sensor *const this)
{
  sns_rc rv = SNS_RC_SUCCESS;
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  lsm6dso_shared_state *shared_state = lsm6dso_get_shared_state(this);

  if((NULL != shared_state->suid_stream &&
      0 != shared_state->suid_stream->api->get_input_cnt(shared_state->suid_stream)) ||
     (NULL != state->reg_data_stream &&
      0 != state->reg_data_stream->api->get_input_cnt(state->reg_data_stream))
#if LSM6DSO_DAE_ENABLED
     || (NULL != shared_state->dae_stream &&
      0 != shared_state->dae_stream->api->get_input_cnt(shared_state->dae_stream))
#endif
    )
  {
    lsm6dso_exit_island(this);
    lsm6dso_process_suid_events(this);
    lsm6dso_process_registry_events(this);
    lsm6dso_dae_if_process_sensor_events(this);
  }
#if !LSM6DSO_POWERRAIL_DISABLED
  rv = lsm6dso_process_timer_events(this);
#endif

  if(rv == SNS_RC_SUCCESS && LSM6DSO_ACCEL == state->sensor)
  {
#if LSM6DSO_POWERRAIL_DISABLED
    if(!shared_state->hw_is_present)
    {
      lsm6dso_com_port_info *com_port = &shared_state->inst_cfg.com_port_info;
      /**-----------------Try to enter I3C mode-------------------------*/
      rv = lsm6dso_enter_i3c_mode(NULL, com_port, shared_state->scp_service);
      DBG_PRINTF_EX(LOW, this, "enter_i3c_mode returns %d", rv);

      rv = lsm6dso_discover_hw(this);
    }
#else
    if(!shared_state->hw_is_present &&
       NULL != shared_state->pwr_rail_service &&
       NULL != shared_state->timer_stream &&
       shared_state->power_rail_pend_state == LSM6DSO_POWER_RAIL_PENDING_NONE)
    {
      sns_time timeticks;
      lsm6dso_update_rail_vote(this, shared_state, SNS_RAIL_ON_LPM, NULL);
      timeticks = sns_convert_ns_to_ticks(LSM6DSO_OFF_TO_IDLE_MS * 1000 * 1000);
      lsm6dso_start_power_rail_timer(this, timeticks, LSM6DSO_POWER_RAIL_PENDING_INIT,
                                     shared_state);
    }
#endif
    if(!state->available && shared_state->hw_is_present && state->outstanding_reg_requests == 0)
    {
      lsm6dso_exit_island(this);
      lsm6dso_update_siblings(this, shared_state);
      lsm6dso_dae_if_check_support(this);
    }
  }

  return rv;
}

static void  lsm6dso_send_flush_config(
  sns_sensor          *const this,
  sns_sensor_instance *instance,
  lsm6dso_sensor_type sensor)
{
  sns_request config;

  config.message_id = SNS_STD_MSGID_SNS_STD_FLUSH_REQ;
  config.request_len = sizeof(sensor);
  config.request = &sensor;

  this->instance_api->set_client_config(instance, &config);
}


/**
 * Returns decoded request message for type
 * sns_std_sensor_config.
 *
 * @param[in] in_request   Request as sotred in client_requests
 *                         list.
 * @param decoded_request  Standard decoded message.
 * @param decoded_payload  Decoded stream request payload.
 *
 * @return bool true if decode is successful else false
 */
static bool lsm6dso_get_decoded_imu_request(
  sns_sensor const      *this,
  sns_request const     *in_request,
  sns_std_request       *decoded_request,
  sns_std_sensor_config *decoded_payload)
{

  pb_istream_t stream;
  pb_simple_cb_arg arg =
      { .decoded_struct = decoded_payload,
        .fields = sns_std_sensor_config_fields };
  decoded_request->payload = (struct pb_callback_s)
      { .funcs.decode = &pb_decode_simple_cb, .arg = &arg };
  stream = pb_istream_from_buffer(in_request->request,
                                  in_request->request_len);
  if(!pb_decode(&stream, sns_std_request_fields, decoded_request))
  {
    SNS_PRINTF(ERROR, this, "decode err");
    return false;
  }
  return true;
}

/**
 * Returns decoded request message for type
 * sns_physical_sensor_test_config_fields.
 *
 * @param[in] in_request   Request as sotred in client_requests
 *                         list.
 * @param decoded_request  Standard decoded message.
 * @param decoded_payload  Decoded stream request payload.
 *
 * @return bool true if decode is successful else false
 */
bool lsm6dso_get_decoded_self_test_request(
  sns_sensor const                *this,
  sns_request const               *in_request,
  sns_std_request                 *decoded_request,
  sns_physical_sensor_test_config *decoded_payload)
{
  pb_simple_cb_arg arg =
      { .decoded_struct = decoded_payload,
        .fields = sns_physical_sensor_test_config_fields };
  decoded_request->payload = (struct pb_callback_s)
      { .funcs.decode = &pb_decode_simple_cb, .arg = &arg };

  pb_istream_t stream = pb_istream_from_buffer(in_request->request, in_request->request_len);
  if(!pb_decode(&stream, sns_std_request_fields, decoded_request))
  {
    SNS_PRINTF(ERROR, this, "decode err - self test");
    return false;
  }
  return true;
}

void lsm6dso_set_client_config(
  sns_sensor           *this,
  sns_sensor_instance  *instance,
  lsm6dso_shared_state *shared_state)
{
  sns_request req_config;
  lsm6dso_instance_state *inst_state = (lsm6dso_instance_state*)instance->state->state;
  if(shared_state->inst_cfg.selftest.requested && !inst_state->self_test_info.test_alive)
  {
    req_config.message_id = SNS_PHYSICAL_SENSOR_TEST_MSGID_SNS_PHYSICAL_SENSOR_TEST_CONFIG;
  }
  else
  {
    req_config.message_id = SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG;
  }

  DBG_PRINTF(MED, this, "set_client_config: msg=%u config=0x%x",
             req_config.message_id, shared_state->inst_cfg.config_sensors);

  LSM6DSO_AUTO_DEBUG_PRINTF(HIGH, this, "set_client_config: msg=%u", req_config.message_id);
  req_config.request_len = sizeof(lsm6dso_instance_config);
  req_config.request = &shared_state->inst_cfg;
  this->instance_api->set_client_config(instance, &req_config);

  if(SNS_PHYSICAL_SENSOR_TEST_MSGID_SNS_PHYSICAL_SENSOR_TEST_CONFIG ==
     req_config.message_id)
  {
    shared_state->inst_cfg.selftest.requested = false;
  }
  else
  {
    shared_state->inst_cfg.config_sensors = 0; // consumed by instance, can be cleared
  }
}

#if !LSM6DSO_POWERRAIL_DISABLED
static void lsm6dso_check_rail_vote(sns_sensor *this)
{
  lsm6dso_shared_state *shared_state = lsm6dso_get_shared_state(this);
  //sns_power_rail_state new_vote = shared_state->rail_config.rail_vote;
  sns_power_rail_state new_vote = SNS_RAIL_ON_LPM;

  if(LSM6DSO_GYRO & shared_state->inst_cfg.client_present ||
     LSM6DSO_GYRO & shared_state->inst_cfg.selftest_client_present)
  {
    new_vote = SNS_RAIL_ON_NPM;
  }
  //TODO need update
#if 0
//#if LSM6DSO_OIS_ENABLED
  if(LSM6DSO_IS_OIS_REQ(shared_state->inst_cfg)) {
    new_vote = SNS_RAIL_ON_NPM;
  }
#endif

  if(shared_state->rail_config.rail_vote != new_vote )
  {
    DBG_PRINTF(MED, this, "check_rail_vote: %u->%u",
               shared_state->rail_config.rail_vote, new_vote);
    lsm6dso_update_rail_vote(lsm6dso_get_master_sensor(this), shared_state, new_vote, NULL);
  }
}
#endif

static void lsm6dso_process_imu_request(
  sns_sensor          *this, 
  sns_request const   *request)
{
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  lsm6dso_combined_request *cimu = &state->combined_imu;
  sns_std_request decoded_request = sns_std_request_init_default;
  sns_std_sensor_config decoded_payload = sns_std_sensor_config_event_init_default;

  if(lsm6dso_get_decoded_imu_request(this, request, &decoded_request, &decoded_payload))
  {
    bool max_batch  = false;
    bool flush_only = false;
    bool is_passive = false;
    UNUSED_VAR(is_passive);
    float dae_report_rate = decoded_payload.sample_rate;
    uint64_t flush_period_ticks = UINT64_MAX;
    uint32_t flush_period = UINT32_MAX;
    uint32_t report_period_us = (uint32_t)(1000000.0f / decoded_payload.sample_rate);
    uint32_t dae_report_period_us = report_period_us;

    if(decoded_request.has_batching)
    {
      if(decoded_request.batching.has_flush_period)
      {
        flush_period = decoded_request.batching.flush_period;
      }
      else if(decoded_request.batching.batch_period > 0)
      {
        flush_period = decoded_request.batching.batch_period;
      }

      if(decoded_request.batching.batch_period > 0)
      {
        dae_report_period_us = report_period_us = decoded_request.batching.batch_period;
      }

      flush_only = (decoded_request.batching.has_flush_only && decoded_request.batching.flush_only);
      if(!flush_only)
      {
        max_batch  = (decoded_request.batching.has_max_batch && decoded_request.batching.max_batch);
      }
      if(flush_only || flush_period == 0)
      {
        dae_report_period_us = UINT32_MAX;
        dae_report_rate = 0.0f;
      }
      else
      {
        dae_report_rate = (1000000.0f / (float)dae_report_period_us);
      }

      flush_period_ticks = sns_convert_ns_to_ticks((uint64_t)flush_period*1000);
    }

    cimu->max_batch         &= max_batch;
    cimu->flush_only        &= flush_only;
    if(!max_batch)
    {
      if(cimu->report_per_us > 0 && report_period_us > 0)
        cimu->report_per_us = SNS_MIN(cimu->report_per_us, report_period_us);
      else if(cimu->report_per_us == 0)
        cimu->report_per_us = report_period_us;

      cimu->report_rate      = (1000000.0f / (float)cimu->report_per_us);
      cimu->dae_report_rate  = SNS_MAX(cimu->dae_report_rate, dae_report_rate);
    }
    cimu->flush_period_ticks = SNS_MAX(cimu->flush_period_ticks, flush_period_ticks);
    cimu->sample_rate        = SNS_MAX(cimu->sample_rate, decoded_payload.sample_rate);

    if(cimu->sample_rate)
    {
      if(request->message_id == SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG)
      {
#if LSM6DSO_PASSIVE_SENSOR_SUPPORT
        if(decoded_request.has_is_passive && decoded_request.is_passive)
        {
          cimu->passive_ngated_client_present = is_passive = true;
        }
        else
#endif
        cimu->ngated_client_present = true;
      }
      else
      {
        cimu->gated_client_present = true;
      }
    }

    DBG_PRINTF(
      HIGH, this, "imu: msg=%u SR*1k=%d RR*1k=%d/%d BP/FP(us)=%d/%d MB/FO/Psv=%03x",
      request->message_id, (int)(decoded_payload.sample_rate*1000), 
      (int)(1000000000UL/report_period_us), (int)(dae_report_rate*1000),
      decoded_request.has_batching ? decoded_request.batching.batch_period : -1,
      decoded_request.batching.has_flush_period ? decoded_request.batching.flush_period : -1,
      ((uint16_t)max_batch << 8) | ((uint16_t)flush_only << 4) | (uint16_t)is_passive);

    LSM6DSO_AUTO_DEBUG_PRINTF(
      HIGH, this,  "sr*1000=%d rr*1000=%d",
      (int)decoded_payload.sample_rate,(int)(dae_report_rate*1000));
  }
}

static void lsm6dso_process_self_test_request(
  sns_sensor           *this,
  sns_request const    *request,
  lsm6dso_shared_state *shared_state)
{
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  sns_std_request decoded_request;
  sns_physical_sensor_test_config decoded_payload =
    sns_physical_sensor_test_config_init_default;

  if (lsm6dso_get_decoded_self_test_request(this, request, &decoded_request, &decoded_payload))
  {
    shared_state->inst_cfg.selftest.requested  = true;
    shared_state->inst_cfg.selftest.test_type  = decoded_payload.test_type;
    shared_state->inst_cfg.selftest.sensor     = state->sensor;
    shared_state->inst_cfg.selftest_client_present |= state->sensor;
  }
}

static void lsm6dso_process_on_change_config(sns_sensor *this)
{
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  lsm6dso_combined_request *cimu = &state->combined_imu;
  lsm6dso_shared_state *shared_state = lsm6dso_get_shared_state(this);
  // Enable MD interrupt when:
  // 1. There is a new request and MD is in MDF state OR
  // 2. There is an existing request and MD is in MDE/MDD state
  uint8_t md_odr = (shared_state->inst_cfg.min_odr_idx << 4);
  float md_odr_val = lsm6dso_get_accel_odr(md_odr);

  if(!cimu->ngated_client_present)
  {
    cimu->ngated_client_present = true;
    cimu->report_rate        = 1.0f/LSM6DSO_MD_REPORT_PERIOD;
    cimu->dae_report_rate    = 0.0f;
    cimu->sample_rate        = md_odr_val;
    cimu->flush_period_ticks = 0;
  }
  DBG_PRINTF(
    LOW, this, "on_change: SR=%u RR*1000=%u cl(ng/g)=%u/%u",
    (int)cimu->sample_rate, (int)(cimu->report_rate*1000),
    cimu->ngated_client_present, cimu->gated_client_present);
}

static void lsm6dso_combine_requests(
  sns_sensor           *this,
  sns_sensor_instance  *instance,
  lsm6dso_shared_state *shared_state)
{
  UNUSED_VAR(shared_state);
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  lsm6dso_combined_request *cimu = &state->combined_imu;
  sns_request const *request;

  sns_memzero(cimu, sizeof(*cimu));
  cimu->max_batch = cimu->flush_only = true;

  for(request = instance->cb->get_client_request(instance, &state->my_suid, true);
      NULL != request;
      request = instance->cb->get_client_request(instance, &state->my_suid, false))
  {
    if(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG             == request->message_id ||
       SNS_STD_EVENT_GATED_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG == request->message_id)
    {
      lsm6dso_process_imu_request(this, request);
    }
    else if(SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG == request->message_id)
    {
      lsm6dso_process_on_change_config(this);
    }
  }
  if(0.0f == cimu->sample_rate)
  {
    cimu->max_batch = cimu->flush_only = false;
  }
  if(cimu->max_batch && state->sensor != LSM6DSO_MOTION_DETECT)
  {
    cimu->report_rate = cimu->dae_report_rate = (1.0f / (float)UINT32_MAX);
    cimu->flush_period_ticks = UINT64_MAX;
  }

  DBG_PRINTF(
    MED, this, "combine: sens=0x%x SR=%u RR*1k=%d MB=%u FO=%u fl_per=%u cl(ng/g/p)=%03x",
    state->sensor, (int)cimu->sample_rate, (int)(cimu->report_rate*1000),
    cimu->max_batch, cimu->flush_only, (uint32_t)cimu->flush_period_ticks,
    ((uint16_t)cimu->ngated_client_present<<8) | ((uint16_t)cimu->gated_client_present<<4) |
    (uint16_t)cimu->passive_ngated_client_present);
}

static void lsm6dso_update_shared_imu(sns_sensor *this, lsm6dso_shared_state *shared_state)
{
  sns_sensor *lib_sensor;
  sns_sensor *accel_sensor;
  lsm6dso_instance_config *inst_cfg = &shared_state->inst_cfg;
  sns_sensor_instance *instance = sns_sensor_util_get_shared_instance(this);
  sns_sensor_uid* suid = &(sns_sensor_uid)ACCEL_SUID_0;
#if LSM6DSO_DUAL_SENSOR_ENABLED
  if(shared_state->hw_idx)
    suid = &(sns_sensor_uid)ACCEL_SUID_1;
#endif

  inst_cfg->sample_rate           = 0.0f;
  inst_cfg->report_rate           = 0.0f;
  inst_cfg->dae_report_rate       = 0.0f;
  inst_cfg->flush_period_ticks    = 0;
  inst_cfg->client_present        = 0;
  inst_cfg->passive_client_present= 0;
  inst_cfg->gated_client_present  = 0;
  inst_cfg->fifo_enable           = 0;

  lib_sensor = this->cb->get_library_sensor(this, true);

  if((((lsm6dso_state*)lib_sensor->state->state)->sensor & (LSM6DSO_MOTION_DETECT | LSM6DSO_ACCEL)) &&
     ((accel_sensor = lsm6dso_get_master_sensor(this)) != NULL) && NULL != instance)
  {
    lsm6dso_state *accel_state = (lsm6dso_state*)accel_sensor->state->state;
    lsm6dso_combined_request *accel_cimu = &accel_state->combined_imu;
    sns_request *request;

    /** Parse through existing accel requests and find gated & non gated requests.
     * These may be out of sync since the sensor instance can auto-promote gated requsets
     * to non-gated requests, and has no way to tell the sensor about the change */

    accel_cimu->ngated_client_present = false;
    accel_cimu->gated_client_present = false;
    accel_cimu->passive_ngated_client_present = false;

    for(request = (sns_request *)instance->cb->get_client_request(instance, suid, true);
        NULL != request;
        request = (sns_request *)instance->cb->get_client_request(instance, suid, false))
    {
      if(accel_cimu->sample_rate != 0)
      {
        if(request->message_id == SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG)
        {
#if LSM6DSO_PASSIVE_SENSOR_SUPPORT
          sns_std_request decoded_request = sns_std_request_init_default;
          sns_std_sensor_config decoded_payload = sns_std_sensor_config_event_init_default;
          lsm6dso_get_decoded_imu_request(this, request, &decoded_request, &decoded_payload);
          if(decoded_request.has_is_passive && decoded_request.is_passive)
          {
            accel_cimu->passive_ngated_client_present = true;
          }
          else
#endif
          accel_cimu->ngated_client_present = true;
        }
        else
        {
          accel_cimu->gated_client_present = true;
        }
      }
    }
  }

  for(lib_sensor = this->cb->get_library_sensor(this, true);
      NULL != lib_sensor;
      lib_sensor = this->cb->get_library_sensor(this, false))
  {
    lsm6dso_state *lib_state = (lsm6dso_state*)lib_sensor->state->state;
    lsm6dso_combined_request *cimu = &lib_state->combined_imu;

    if(LSM6DSO_SENSOR_TEMP == lib_state->sensor)
    {
      inst_cfg->temper_sample_rate = cimu->sample_rate;
      inst_cfg->temper_report_rate = cimu->report_rate;
    }

    // From sns_std.proto regarding is_passive:
    // Actively enabling one sensor shall not lead to enabling of another
    // sensor having only passive requests.
    if(cimu->ngated_client_present || cimu->gated_client_present)
    {
      inst_cfg->sample_rate = SNS_MAX(inst_cfg->sample_rate, cimu->sample_rate);
      inst_cfg->report_rate = SNS_MAX(inst_cfg->report_rate, cimu->report_rate);
      inst_cfg->dae_report_rate = SNS_MAX(inst_cfg->dae_report_rate, cimu->dae_report_rate);
      inst_cfg->flush_period_ticks = SNS_MAX(inst_cfg->flush_period_ticks,
                                             cimu->flush_period_ticks);
    }

    if(cimu->ngated_client_present)
    {
      inst_cfg->client_present |= lib_state->sensor;

      if(lib_state->sensor == LSM6DSO_GYRO)
      {
        inst_cfg->fifo_enable |= (LSM6DSO_GYRO | LSM6DSO_ACCEL);
      }
      else if(lib_state->sensor == LSM6DSO_ACCEL)
      {
        inst_cfg->fifo_enable |= LSM6DSO_ACCEL;
      }
    }

    if(cimu->gated_client_present)
    {
      //special case if gated client is present
      //do not set publish sensors if gated, it is handled while sending data
      inst_cfg->fifo_enable          |= LSM6DSO_ACCEL;
      inst_cfg->gated_client_present |= lib_state->sensor;
    }

    if(cimu->passive_ngated_client_present)
    {
      inst_cfg->passive_client_present |= lib_state->sensor;
    }

    LSM6DSO_AUTO_DEBUG_PRINTF(
      HIGH, this, "config: %d %d %d",
      lib_state->sensor, (int)(cimu->sample_rate*1000), (int)(cimu->report_rate*1000));
  }

  DBG_PRINTF(HIGH, this, "shared_imu: SR*1k=%d RR*1k=%d/%d fl_per(ticks)=%u",
            (int)(inst_cfg->sample_rate*1000), (int)(inst_cfg->report_rate*1000),
            (int)(inst_cfg->dae_report_rate*1000), (uint32_t)inst_cfg->flush_period_ticks);
  DBG_PRINTF(HIGH, this, "shared_imu: clients(ng/g/psv/st)=0x%x/%x/%x/%x fifo=0x%x",
             inst_cfg->client_present, inst_cfg->gated_client_present,
             inst_cfg->passive_client_present, inst_cfg->selftest_client_present,
             inst_cfg->fifo_enable);
}

static void lsm6dso_reval(
  sns_sensor          *const this,
  sns_sensor_instance *instance,
  sns_request const   *request)
{
  lsm6dso_shared_state *shared_state = lsm6dso_get_shared_state(this);
  lsm6dso_combine_requests(this, instance, shared_state);

  if(SNS_PHYSICAL_SENSOR_TEST_MSGID_SNS_PHYSICAL_SENSOR_TEST_CONFIG !=
     request->message_id)
  {
    lsm6dso_state *state = (lsm6dso_state*)this->state->state;
    if(state->sensor == LSM6DSO_OIS) {
      shared_state->inst_cfg.config_sensors |= state->sensor;
      lsm6dso_update_ois_sensor_config(this, instance);
    } else {
      lsm6dso_update_shared_imu(this, shared_state);
    }
  }
  else
  {
    lsm6dso_process_self_test_request(this, request, shared_state);
  }

#if LSM6DSO_POWERRAIL_DISABLED
  lsm6dso_set_client_config(this, instance, shared_state);
#else
  lsm6dso_check_rail_vote(this); // as Gyro clients might have come or gone

  if(SNS_RAIL_OFF != shared_state->rail_config.rail_vote &&
     LSM6DSO_POWER_RAIL_PENDING_NONE == shared_state->power_rail_pend_state)
  {
    lsm6dso_set_client_config(this, instance, shared_state);
  }
  else
  {
    DBG_PRINTF_EX(MED, this, "reval: rail=%u pending=%u",
               shared_state->rail_config.rail_vote, shared_state->power_rail_pend_state);

  }
#endif
}

static void lsm6dso_remove_request(
  sns_sensor               *const this,
  sns_sensor_instance      *instance,
  lsm6dso_shared_state     *shared_state,
  struct sns_request const *exist_request)
{
  if(NULL != instance)
  {
    lsm6dso_state *state = (lsm6dso_state*)this->state->state;

    /* Assumption: The FW will call deinit() on the instance before destroying it.
       Putting all HW resources (sensor HW, COM port, power rail)in
       low power state happens in Instance deinit().*/
    if(exist_request->message_id != SNS_PHYSICAL_SENSOR_TEST_MSGID_SNS_PHYSICAL_SENSOR_TEST_CONFIG)
    {
      shared_state->inst_cfg.config_sensors |= state->sensor;
      lsm6dso_reval(this, instance, exist_request);
    }
    else
    {
      shared_state->inst_cfg.selftest_client_present &= ~state->sensor;
      lsm6dso_exit_island(this);
      lsm6dso_handle_selftest_request_removal(this, instance, shared_state);
    }
#if !LSM6DSO_POWERRAIL_DISABLED
    if(shared_state->inst_cfg.client_present == 0 &&
       shared_state->inst_cfg.passive_client_present == 0 &&
       shared_state->inst_cfg.gated_client_present == 0 &&
       shared_state->inst_cfg.selftest_client_present == 0)
    {
      lsm6dso_turn_rails_off(this, shared_state);
    }
#endif
  }
}

static sns_rc lsm6dso_add_new_request(
  sns_sensor               *const this,
  sns_sensor_instance      *instance,
  lsm6dso_shared_state     *shared_state,
  struct sns_request const *new_request)
{
  sns_rc rc = SNS_RC_SUCCESS;
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;

  if(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG             == new_request->message_id ||
     SNS_STD_EVENT_GATED_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG == new_request->message_id)
  {
    sns_std_request decoded_request;
    sns_std_sensor_config decoded_payload = {0};
    if(lsm6dso_get_decoded_imu_request(this, new_request, &decoded_request,
                                       &decoded_payload) &&
       0.0f < decoded_payload.sample_rate &&
       ((LSM6DSO_SENSOR_TEMP == state->sensor &&
         LSM6DSO_SENSOR_TEMP_ODR_5 >= decoded_payload.sample_rate) ||
        (LSM6DSO_SENSOR_TEMP != state->sensor &&
         MAX_LOW_LATENCY_RATE >= decoded_payload.sample_rate)))
    {
      shared_state->inst_cfg.config_sensors |= state->sensor;
#if LSM6DSO_FORCE_SENSOR_TEMP_ENABLED
      if (state->sensor & (LSM6DSO_ACCEL | LSM6DSO_GYRO))
      {
        shared_state->inst_cfg.config_sensors |= LSM6DSO_SENSOR_TEMP;
      }
#endif
    }
    else
    {
      rc = SNS_RC_INVALID_VALUE;

      SNS_PRINTF(ERROR, this, "new_request: sensor=%u SR=%d",
                 state->sensor, (int)decoded_payload.sample_rate);
    }
  }
  else if(SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG == new_request->message_id)
  {
    if(LSM6DSO_MOTION_DETECT != state->sensor && !LSM6DSO_IS_ESP_SENSOR(state->sensor))
    {
      rc = SNS_RC_INVALID_VALUE;
    }
    else if(LSM6DSO_MOTION_DETECT == state->sensor)
    {
      lsm6dso_instance_state *inst_state = (lsm6dso_instance_state*)instance->state->state;
      inst_state->md_info.add_request = true;
      shared_state->inst_cfg.config_sensors |= state->sensor;
    }
    if(LSM6DSO_IS_ESP_SENSOR(state->sensor))
    {
      shared_state->inst_cfg.config_sensors |= state->sensor;
    }

  }
  else if(SNS_PHYSICAL_SENSOR_TEST_MSGID_SNS_PHYSICAL_SENSOR_TEST_CONFIG ==
          new_request->message_id)
  {
    sns_std_request decoded_request;
    sns_physical_sensor_test_config decoded_payload =
      sns_physical_sensor_test_config_init_default;
    if (!lsm6dso_get_decoded_self_test_request(this, new_request, &decoded_request,
                                               &decoded_payload) ||
        (state->sensor & (LSM6DSO_ACCEL | LSM6DSO_GYRO) &&
         decoded_payload.test_type == SNS_PHYSICAL_SENSOR_TEST_TYPE_SW) ||
        (state->sensor & (LSM6DSO_MOTION_DETECT | LSM6DSO_SENSOR_TEMP) &&
         decoded_payload.test_type != SNS_PHYSICAL_SENSOR_TEST_TYPE_COM))
    {
      rc = SNS_RC_INVALID_VALUE;
    }
    else if(LSM6DSO_IS_ESP_SENSOR(state->sensor))
    {
      shared_state->inst_cfg.config_sensors |= state->sensor;
    }
  }
  else
  {
    rc = SNS_RC_INVALID_VALUE;
  }

  if(rc == SNS_RC_SUCCESS)
  {
    lsm6dso_reval(this, instance, new_request);
  }
  else
  {
    sns_sensor_uid *suid = &state->my_suid;
    sns_std_error_event error_event; 
    error_event.error = SNS_STD_ERROR_INVALID_VALUE; 
    pb_send_event(instance, 
                 sns_std_error_event_fields, 
                 &error_event, 
                 sns_get_system_time(), 
                 SNS_STD_MSGID_SNS_STD_ERROR_EVENT, 
                 suid);

    instance->cb->remove_client_request(instance, new_request);
    SNS_PRINTF(ERROR, this, "req rejec, rc=%d", rc);
  }
  return rc;
}

sns_sensor_instance *create_new_instance(sns_sensor *const this)
{
  sns_sensor_instance *instance = NULL;
  sns_sensor *master_sensor = lsm6dso_get_master_sensor(this);
  if(NULL != master_sensor)
  {
#if !LSM6DSO_POWERRAIL_DISABLED
    lsm6dso_shared_state *shared_state = lsm6dso_get_shared_state(this);
    lsm6dso_state *state = (lsm6dso_state*)this->state->state;
    sns_time on_timestamp, delta;
    sns_time off2idle = sns_convert_ns_to_ticks(LSM6DSO_OFF_TO_IDLE_MS*1000*1000);

    lsm6dso_update_rail_vote(
      master_sensor, shared_state,
      state->sensor == LSM6DSO_GYRO ? SNS_RAIL_ON_NPM : SNS_RAIL_ON_LPM,
      &on_timestamp);

    delta = sns_get_system_time() - on_timestamp;

    // Use on_timestamp to determine correct Timer value.
    if(delta < off2idle)
    {
      DBG_PRINTF_EX(MED, this, "new_inst: start power rail timer");
      lsm6dso_start_power_rail_timer(
        this,
        off2idle - delta,
        LSM6DSO_POWER_RAIL_PENDING_SET_CLIENT_REQ,
        shared_state);
    }
    else
    {
      DBG_PRINTF_EX(MED, this, "new_inst: rail already ON");
      shared_state->power_rail_pend_state = LSM6DSO_POWER_RAIL_PENDING_NONE;
      sns_sensor_util_remove_sensor_stream(this, &shared_state->timer_stream);
    }
#endif

    // must create instance from master sensor whose state includes the shared state
    DBG_PRINTF_EX(MED, this, "creating inst");
    instance = master_sensor->cb->create_instance(master_sensor,
                                                  sizeof(lsm6dso_instance_state));
  }
  return instance;
}

static void lsm6dso_handle_flush_request(
  sns_sensor           *this,
  sns_sensor_instance  *instance,
  lsm6dso_shared_state *shared_state)
{
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  lsm6dso_flush_done_reason reason = FLUSH_TO_BE_DONE;

#if !LSM6DSO_POWERRAIL_DISABLED
  if(SNS_RAIL_OFF == shared_state->rail_config.rail_vote ||
     LSM6DSO_POWER_RAIL_PENDING_SET_CLIENT_REQ == shared_state->power_rail_pend_state)
  {
    DBG_PRINTF(MED, this, "handle_flush_request: sensor=%u", state->sensor);
    state->flush_req = true;
    return;
  }
  else
#endif
  if(state->sensor & (LSM6DSO_MOTION_DETECT | LSM6DSO_SENSOR_TEMP))
  {
    reason = FLUSH_DONE_NOT_ACCEL_GYRO;
  }
  else if(shared_state->inst_cfg.fifo_enable == 0)
  {
    reason = FLUSH_DONE_NOT_FIFO;
  }

  if(reason != FLUSH_TO_BE_DONE)
  {
    lsm6dso_send_fifo_flush_done(instance, state->sensor, reason);
  }
  else
  {
    lsm6dso_send_flush_config(this, instance, state->sensor);
  }
}

#if !LSM6DSO_REGISTRY_DISABLED
static void  lsm6dso_send_cal_config(
  sns_sensor                     *this,
  sns_sensor_instance            *instance,
  sns_lsm6dso_registry_cfg const *cal)
{
  sns_request config;

  config.message_id = SNS_CAL_MSGID_SNS_CAL_RESET;
  config.request_len = sizeof(*cal);
  config.request = (void*)cal;

  this->instance_api->set_client_config(instance, &config);
}
#endif
static void lsm6dso_reset_calibration(
  sns_sensor           *this,
  sns_sensor_instance  *instance,
  lsm6dso_shared_state *shared_state)
{
#if !LSM6DSO_REGISTRY_DISABLED
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;

  if(state->sensor & (LSM6DSO_ACCEL | LSM6DSO_GYRO))
  {
    sns_lsm6dso_registry_cfg *cal = (LSM6DSO_ACCEL == state->sensor) ?
      &shared_state->inst_cfg.accel_cal : &shared_state->inst_cfg.gyro_cal;

    sns_memzero(&cal->fac_cal_bias, sizeof(cal->fac_cal_bias));
    sns_memzero(&cal->fac_cal_corr_mat, sizeof(cal->fac_cal_corr_mat));
    cal->fac_cal_corr_mat.e00 = 1.0f;
    cal->fac_cal_corr_mat.e11 = 1.0f;
    cal->fac_cal_corr_mat.e22 = 1.0f;
    cal->registry_persist_version++;
    cal->thermal_scale.x = 0.0f;
    cal->thermal_scale.y = 0.0f;
    cal->thermal_scale.z = 0.0f;
    cal->ts = sns_get_system_time();

    lsm6dso_send_cal_config(this, instance, cal);

    DBG_PRINTF_EX(HIGH, this, "reset_calibration: ver=%u", cal->registry_persist_version);
    lsm6dso_exit_island(this);
    lsm6dso_sensor_write_output_to_registry(this);
  }
#else
  UNUSED_VAR(this);
  UNUSED_VAR(instance);
  UNUSED_VAR(shared_state);
#endif
}

bool lsm6dso_is_request_present(sns_sensor_instance *instance, uint8_t hw_idx )
{
#if LSM6DSO_DUAL_SENSOR_ENABLED
  if(hw_idx) {
    if(NULL != instance->cb->get_client_request(instance,
          &(sns_sensor_uid)ACCEL_SUID_1, true) ||
        NULL != instance->cb->get_client_request(instance,
          &(sns_sensor_uid)MOTION_DETECT_SUID_1, true) ||
        NULL != instance->cb->get_client_request(instance,
          &(sns_sensor_uid)GYRO_SUID_1, true) ||
        NULL != instance->cb->get_client_request(instance,
          &(sns_sensor_uid)SENSOR_TEMPERATURE_SUID_1, true)
      )
      return true;
  } else
#endif
  {
    UNUSED_VAR(hw_idx);
    if(NULL != instance->cb->get_client_request(instance,
          &(sns_sensor_uid)ACCEL_SUID_0, true) ||
        NULL != instance->cb->get_client_request(instance,
          &(sns_sensor_uid)MOTION_DETECT_SUID_0, true) ||
        NULL != instance->cb->get_client_request(instance,
          &(sns_sensor_uid)GYRO_SUID_0, true) ||
        NULL != instance->cb->get_client_request(instance,
          &(sns_sensor_uid)SENSOR_TEMPERATURE_SUID_0, true)
      )
      return true;
  }
  //No request for primary sensors

  if(lsm6dso_is_esp_request_present(instance))
    return true;

  if(lsm6dso_is_ois_request_present(instance))
    return true;

  return false;
}
sns_sensor_instance* lsm6dso_update_request_q(sns_sensor *const this,
                              struct sns_request const *exist_request,
                              struct sns_request const *new_request,
                              bool remove)
{
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  UNUSED_VAR(state);
  sns_sensor_instance *instance = sns_sensor_util_get_shared_instance(this);
  if(remove && (NULL != exist_request) && (NULL != instance))
  {
    instance->cb->remove_client_request(instance, exist_request);
  } else if(!remove && NULL != new_request) {
    if (NULL == instance &&
        // first request cannot be a Flush request or Calibration reset request
        SNS_STD_MSGID_SNS_STD_FLUSH_REQ != new_request->message_id)
    {
      instance = create_new_instance(this);
    }
    if(instance) {
      lsm6dso_instance_state *inst_state = (lsm6dso_instance_state*)instance->state->state;

      if(!inst_state->self_test_info.test_alive && 
          SNS_STD_MSGID_SNS_STD_FLUSH_REQ != new_request->message_id &&
          SNS_CAL_MSGID_SNS_CAL_RESET != new_request->message_id) {
        if (NULL != exist_request)
        {
          DBG_PRINTF(LOW, this, "replace req");
          instance->cb->remove_client_request(instance, exist_request);
        }
        instance->cb->add_client_request(instance, new_request);
        DBG_PRINTF(LOW, this, "adding req to Q sensor=%d req=%p msg=%d", state->sensor, new_request, new_request->message_id);
      }
    }
  }
  return instance;
}

sns_sensor_instance* lsm6dso_handle_client_request(sns_sensor *const this,
                                   struct sns_request const *exist_request,
                                   struct sns_request const *new_request,
                                   bool remove)
{
  sns_sensor_instance *instance = sns_sensor_util_get_shared_instance(this);
  lsm6dso_shared_state *shared_state = lsm6dso_get_shared_state(this);
  bool flush_req_handled = false;
  
  if(remove && (NULL != exist_request))
  {
    lsm6dso_remove_request(this, instance, shared_state, exist_request);
  }
  else if(NULL != new_request)
  {
    // 1. If new request then:
    //     a. Power ON rails.
    //     b. Power ON COM port - Instance must handle COM port power.
    //     c. Create new instance.
    //     d. Re-evaluate existing requests and choose appropriate instance config.
    //     e. set_client_config for this instance.
    //     f. Add new_request to list of requests handled by the Instance.
    //     g. Power OFF COM port if not needed- Instance must handle COM port power.
    //     h. Return the Instance.
    // 2. If there is an Instance already present:
    //     a. Add new_request to list of requests handled by the Instance.
    //     b. Remove exist_request from list of requests handled by the Instance.
    //     c. Re-evaluate existing requests and choose appropriate Instance config.
    //     d. set_client_config for the Instance if not the same as current config.
    //     e. publish the updated config.
    //     f. Return the Instance.
    // 3.  If "flush" request:
    //     a. Perform flush on the instance.
    //     b. Return NULL.

    if (NULL != instance)
    {
      lsm6dso_instance_state *inst_state = (lsm6dso_instance_state*)instance->state->state;
      if(!inst_state->self_test_info.test_alive)
      {
        if(SNS_STD_MSGID_SNS_STD_FLUSH_REQ == new_request->message_id) // most frequent request
        {
          if(NULL == exist_request)
          {
            SNS_PRINTF(HIGH, this, "orphan Flush req!");
            instance = NULL;
          }
          else
          {
            lsm6dso_handle_flush_request(this, instance, shared_state);
            flush_req_handled = true;
          }
        }
        else if(lsm6dso_is_valid_oem_request(new_request->message_id))
        {
          lsm6dso_handle_oem_request(this, instance, new_request);
          return instance;
        }
        else if(SNS_CAL_MSGID_SNS_CAL_RESET != new_request->message_id)
        {
          if(SNS_RC_SUCCESS != lsm6dso_add_new_request(this, instance, shared_state, new_request))
          {
            if(NULL != exist_request)
            {
              DBG_PRINTF(HIGH, this, "restoring existing req");
              instance->cb->add_client_request(instance, exist_request);
            }
            instance = NULL; // no instance is handling this invalid request
          }
        }
        else // CAL_RESET is the least frequent request
        {
          if(NULL != exist_request)
          {
            lsm6dso_reset_calibration(this, instance, shared_state);
          }
          else
          {
            instance->cb->add_client_request(instance, new_request);
            lsm6dso_reset_calibration(this, instance, shared_state);
            instance->cb->remove_client_request(instance, new_request);
          }
        }
      }
      else
      {
        DBG_PRINTF(HIGH, this, "Selftest running. Reject");
        instance = NULL; // no instance is handling this request
      }
#if LSM6DSO_POWERRAIL_DISABLED
      if(NULL != instance)
      {
        lsm6dso_set_client_config(this, instance, shared_state);
      }
#endif
    }
  }
  else // bad request
  {
    instance = NULL; // no instance is handling this invalid request
  }

  if(NULL != instance && !lsm6dso_is_request_present(instance, shared_state->hw_idx))
  {
    lsm6dso_instance_state *inst_state = (lsm6dso_instance_state*)instance->state->state;
    // must call remove_instance() when clientless
    SNS_PRINTF(MED, this, "client_req: remove instance");
    shared_state->odr_percent_var_accel = inst_state->odr_percent_var_accel;
    shared_state->odr_percent_var_gyro = inst_state->odr_percent_var_gyro;
    this->cb->remove_instance(instance);
    instance = NULL;
  }

  if(NULL != new_request &&
     SNS_CAL_MSGID_SNS_CAL_RESET == new_request->message_id)
  {
    instance = &sns_instance_no_error;
  }
#if LSM6DSO_FLUSH_SPECIAL_HANDLING
  else if(flush_req_handled)
  {
    instance = &sns_instance_no_error;
  }
#endif
  return instance;
}


/** See sns_lsm6dso_sensor.h */
sns_sensor_instance* lsm6dso_set_client_request(sns_sensor *const this,
                                                struct sns_request const *exist_request,
                                                struct sns_request const *new_request,
                                                bool remove)
{
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;

  UNUSED_VAR(state);
  if(new_request == NULL || new_request->message_id != SNS_STD_MSGID_SNS_STD_FLUSH_REQ)
  {
    SNS_PRINTF(HIGH, this, "client_req: sensor=%u req=%d/%d remove=%u hw_id=[%u]",
               state->sensor, exist_request != NULL ? exist_request->message_id : -1,
               new_request != NULL ? new_request->message_id : -1, remove,
               state->hardware_id);
  }
  lsm6dso_update_request_q(this, exist_request, new_request, remove);
  return lsm6dso_handle_client_request(this, exist_request, new_request, remove);
}

sns_sensor* lsm6dso_get_sensor_by_type(sns_sensor *const this, lsm6dso_sensor_type sensor)
{
  sns_sensor *lib_sensor;

  for(lib_sensor = this->cb->get_library_sensor(this, true);
      NULL != lib_sensor;
      lib_sensor = this->cb->get_library_sensor(this, false))
  {
    lsm6dso_state *lib_state = (lsm6dso_state*)lib_sensor->state->state;
    if(lib_state->sensor == sensor)
    {
      break;
    }
  }
  return lib_sensor;
}

sns_sensor* lsm6dso_get_master_sensor(sns_sensor *const this)
{
  return (lsm6dso_get_sensor_by_type(this, LSM6DSO_ACCEL)); // Accel is the master sensor
}

lsm6dso_shared_state* lsm6dso_get_shared_state_from_state(sns_sensor_state const *state)
{
  return (lsm6dso_shared_state*)((intptr_t)(state->state) + ALIGN_8(sizeof(lsm6dso_state)));
}

lsm6dso_shared_state* lsm6dso_get_shared_state(sns_sensor *const this)
{
  lsm6dso_shared_state *shared_state = NULL;
  sns_sensor *master = lsm6dso_get_master_sensor(this);
  if(NULL != master)
  {
    shared_state = lsm6dso_get_shared_state_from_state(master->state);
  }
  return shared_state;
}

lsm6dso_instance_config const* lsm6dso_get_instance_config(sns_sensor_state const *state)
{
  lsm6dso_shared_state *shared_state = lsm6dso_get_shared_state_from_state(state);
  return &shared_state->inst_cfg;
}

void lsm6dso_update_rail_vote(
  sns_sensor            *this,
  lsm6dso_shared_state  *shared_state,
  sns_power_rail_state  vote,
  sns_time              *on_timestamp)
{
#if !LSM6DSO_POWERRAIL_DISABLED

  DBG_PRINTF(MED, this, "update_rail_vote: %u->%u",
      shared_state->rail_config.rail_vote, vote);
  shared_state->rail_config.rail_vote = vote;
  shared_state->pwr_rail_service->api->
    sns_vote_power_rail_update(shared_state->pwr_rail_service,
                               this,
                               &shared_state->rail_config,
                               on_timestamp);
#else
  UNUSED_VAR(this);
  UNUSED_VAR(shared_state);
  UNUSED_VAR(vote);
  UNUSED_VAR(on_timestamp);
#endif
}

/*===========================================================================
  Public Data Definitions
  ===========================================================================*/
sns_sensor_api lsm6dso_accel_sensor_api =
{
  .struct_len         = sizeof(sns_sensor_api),
  .init               = &lsm6dso_accel_init,
  .deinit             = &lsm6dso_accel_deinit,
  .get_sensor_uid     = &lsm6dso_get_sensor_uid,
  .set_client_request = &lsm6dso_set_client_request,
  .notify_event       = &lsm6dso_sensor_notify_event,
};
sns_sensor_api lsm6dso_gyro_sensor_api =
{
  .struct_len         = sizeof(sns_sensor_api),
  .init               = &lsm6dso_gyro_init,
  .deinit             = &lsm6dso_gyro_deinit,
  .get_sensor_uid     = &lsm6dso_get_sensor_uid,
  .set_client_request = &lsm6dso_set_client_request,
  .notify_event       = &lsm6dso_sensor_notify_event,
};
sns_sensor_api lsm6dso_motion_detect_sensor_api =
{
  .struct_len         = sizeof(sns_sensor_api),
  .init               = &lsm6dso_motion_detect_init,
  .deinit             = &lsm6dso_motion_detect_deinit,
  .get_sensor_uid     = &lsm6dso_get_sensor_uid,
  .set_client_request = &lsm6dso_set_client_request,
  .notify_event       = &lsm6dso_sensor_notify_event,
};

sns_sensor_api lsm6dso_sensor_temp_sensor_api =
{
  .struct_len         = sizeof(sns_sensor_api),
  .init               = &lsm6dso_sensor_temp_init,
  .deinit             = &lsm6dso_sensor_temp_deinit,
  .get_sensor_uid     = &lsm6dso_get_sensor_uid,
  .set_client_request = &lsm6dso_set_client_request,
  .notify_event       = &lsm6dso_sensor_notify_event,
};

