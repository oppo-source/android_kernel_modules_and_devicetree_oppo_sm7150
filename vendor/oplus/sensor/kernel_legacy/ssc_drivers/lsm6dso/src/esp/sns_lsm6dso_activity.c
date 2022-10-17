/**
 * @file sns_lsm6dso_activity.c
 *
 * Common implementation for LSM6DSO activity Sensor
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
#include "sns_types.h"
#include "sns_lsm6dso_sensor.h"

#if LSM6DSO_ESP_ACTIVITY
#include "pb_encode.h"
#include "sns_mem_util.h"
#include "sns_pb_util.h"
#include "sns_attribute_util.h"

#define PLATFORM_CONFIG_ACTIVITY       "_platform.act.config"

extern sns_sensor_api lsm6dso_activity_sensor_api;
extern sns_sensor_api lsm6dso_inactivity_sensor_api;

extern void lsm6dso_reconfig_inact(sns_sensor_instance *instance, bool enable);
extern void lsm6dso_reconfig_act(sns_sensor_instance *instance, bool enable);
extern void lsm6dso_handle_inactivity_timer_events(sns_sensor_instance *const instance);
extern void lsm6dso_handle_activity_intr(sns_sensor_instance *const instance, sns_time irq_timestamp,
                                         uint8_t src_reg, uint8_t wake_data);
//extern void lsm6dso_fill_act_inst_info(sns_sensor_instance *instance, sns_sensor_state const *sstate);

static char lsm6dso_act_reg_config[SENSOR_INST_CNT][1][SNS_REGISTRY_MAX_NAME_LEN] = {
  {
    LSM6DSO_GEN_GROUP(0, PLATFORM_CONFIG_ACTIVITY),
  },
#if LSM6DSO_DUAL_SENSOR_ENABLED
  {
    LSM6DSO_GEN_GROUP(1, PLATFORM_CONFIG_ACTIVITY),
  }
#endif
};

#if !LSM6DSO_REGISTRY_DISABLED
void lsm6dso_send_act_registry_requests(sns_sensor *const this, uint8_t hw_id)
{
  lsm6dso_send_registry_request(this, lsm6dso_act_reg_config[hw_id][0]);
}
static bool
lsm6dso_reg_parse_activity_cfg(sns_registry_data_item *reg_item,
                          struct pb_buffer_arg  *item_name,
                          struct pb_buffer_arg  *item_str_val,
                          void *parsed_buffer)
{
  lsm6dso_act_registry_cfg *data_ptr = (lsm6dso_act_registry_cfg *)parsed_buffer;
  UNUSED_VAR(item_str_val);
  if(!reg_item->has_flt)
    return false;
  if(0 == strncmp((char*)item_name->buf,
        "thresh",
        item_name->buf_len)) {
    data_ptr->thresh = reg_item->flt;
  } else if(0 == strncmp((char*)item_name->buf,
        "win",
        item_name->buf_len)) {
    data_ptr->dur = reg_item->flt;
  } else if(0 == strncmp((char*)item_name->buf,
        "inact_timeout",
        item_name->buf_len)) {
    data_ptr->inact_timeout = reg_item->flt;
  }

  return true;
}

void lsm6dso_process_act_registry_event(sns_sensor *const this, sns_sensor_event *event)
{
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  lsm6dso_shared_state *shared_state = lsm6dso_get_shared_state(this);

  pb_istream_t stream = pb_istream_from_buffer((void*)event->event,
      event->event_len);

  if(SNS_REGISTRY_MSGID_SNS_REGISTRY_READ_EVENT == event->message_id)
  {
    sns_registry_read_event read_event = sns_registry_read_event_init_default;
    pb_buffer_arg group_name = {0,0};
    read_event.name.arg = &group_name;
    read_event.name.funcs.decode = pb_decode_string_cb;

    if(!pb_decode(&stream, sns_registry_read_event_fields, &read_event))
    {
      DBG_PRINTF(HIGH, this,  "Error decoding registry event");
    }
    else
    {
      bool rv = true;
      uint32_t hw_id = shared_state->hw_idx;
      stream = pb_istream_from_buffer((void*)event->event, event->event_len);
      if(0 == strncmp((char*)group_name.buf,
            lsm6dso_act_reg_config[hw_id][0],
            group_name.buf_len))
      {
        {
          sns_registry_decode_arg arg = {
            .item_group_name = &group_name,
            .parse_info_len = 1,
            .parse_info[0] = {
              .parse_func = lsm6dso_reg_parse_activity_cfg,
              .parsed_buffer = &shared_state->inst_cfg.esp_reg_cfg.act_reg_conf }
          };

          sns_strlcpy(arg.parse_info[0].group_name,lsm6dso_act_reg_config[hw_id][0],sizeof(arg.parse_info[0].group_name));
          read_event.data.items.funcs.decode = &sns_registry_item_decode_cb;
          read_event.data.items.arg = &arg;

          rv = pb_decode(&stream, sns_registry_read_event_fields, &read_event);
        }

        if(rv)
        {
          DBG_PRINTF_EX(HIGH, this,
              "Activity Threshold(*1000):%d m/s2 Window(*1000):%d sec timeout(*1000) :%d sec",
              (int)(shared_state->inst_cfg.esp_reg_cfg.act_reg_conf.thresh*1000),
              (int) (shared_state->inst_cfg.esp_reg_cfg.act_reg_conf.dur*1000),
              (int) (shared_state->inst_cfg.esp_reg_cfg.act_reg_conf.inact_timeout*1000));
        }
      }
      else
      {
        rv = false;
      }
      if(!rv)
      {
        DBG_PRINTF(HIGH, this,  "Error decoding registry group ");
      }
    }
    state->outstanding_reg_requests--;
  }
}

#endif // !LSM6DSO_REGISTRY_DISABLED
sns_rc lsm6dso_activity_init(sns_sensor *const this)
{
  lsm6dso_state *state = (lsm6dso_state*)this->state->state;
  sns_sensor_uid* suid = &((sns_sensor_uid)ACTIVITY_SUID_0);
  sns_rc rc;
#if LSM6DSO_DUAL_SENSOR_ENABLED
  lsm6dso_shared_state *shared_state = lsm6dso_get_shared_state(this);
  suid = (shared_state->hw_idx) ? &((sns_sensor_uid)ACTIVITY_SUID_1) : suid;
#endif
  rc = lsm6dso_esp_init(this, suid, &lsm6dso_supported_esp_sensors[ACT_INDX], LSM6DSO_ACTIVITY);
  state->send_reg_request = &lsm6dso_send_act_registry_requests; 
  return rc;
}

sns_rc lsm6dso_inactivity_init(sns_sensor *const this)
{
  sns_rc rc;
  sns_sensor_uid* suid = &((sns_sensor_uid)INACTIVITY_SUID_0);
#if LSM6DSO_DUAL_SENSOR_ENABLED
  suid = (shared_state->hw_idx) ? &((sns_sensor_uid)INACTIVITY_SUID_1) : suid;
#endif
  rc = lsm6dso_esp_init(this, suid, &lsm6dso_supported_esp_sensors[INACT_INDX], LSM6DSO_INACTIVITY);
  return rc;
}

sns_rc lsm6dso_activity_deinit(sns_sensor *const this)
{
  UNUSED_VAR(this);
  return SNS_RC_SUCCESS; 
}

sns_rc lsm6dso_inactivity_deinit(sns_sensor *const this)
{
  UNUSED_VAR(this);
  return SNS_RC_SUCCESS; 
}

void lsm6dso_register_inactivity(sns_register_cb const *register_api)
{
   sns_strlcpy(lsm6dso_supported_esp_sensors[INACT_INDX].proto, "sns_activity.proto", sizeof(lsm6dso_supported_esp_sensors[INACT_INDX].proto));
   lsm6dso_supported_esp_sensors[INACT_INDX].sensor = LSM6DSO_INACTIVITY;
   lsm6dso_supported_esp_sensors[INACT_INDX].min_odr = LSM6DSO_ODR_26;
   lsm6dso_supported_esp_sensors[INACT_INDX].stream_type = SNS_STD_SENSOR_STREAM_TYPE_SINGLE_OUTPUT;
   lsm6dso_supported_esp_sensors[INACT_INDX].reconfig = &lsm6dso_reconfig_inact; 
   lsm6dso_supported_esp_sensors[INACT_INDX].handle_intr = NULL;
   lsm6dso_supported_esp_sensors[INACT_INDX].handle_timer_events = &lsm6dso_handle_inactivity_timer_events; 
   register_api->init_sensor(sizeof(lsm6dso_state),
                              &lsm6dso_inactivity_sensor_api,
                              &lsm6dso_sensor_instance_api);
}

void lsm6dso_register_activity(sns_register_cb const *register_api)
{
   sns_strlcpy(lsm6dso_supported_esp_sensors[ACT_INDX].proto, "sns_activity.proto", sizeof(lsm6dso_supported_esp_sensors[ACT_INDX].proto));
   lsm6dso_supported_esp_sensors[ACT_INDX].sensor = LSM6DSO_ACTIVITY;
   lsm6dso_supported_esp_sensors[ACT_INDX].min_odr = LSM6DSO_ODR_26;
   lsm6dso_supported_esp_sensors[ACT_INDX].stream_type = SNS_STD_SENSOR_STREAM_TYPE_SINGLE_OUTPUT;
   lsm6dso_supported_esp_sensors[ACT_INDX].reconfig = &lsm6dso_reconfig_act; 
   lsm6dso_supported_esp_sensors[ACT_INDX].handle_intr = &lsm6dso_handle_activity_intr;
   lsm6dso_supported_esp_sensors[ACT_INDX].handle_timer_events = NULL; 

   register_api->init_sensor(sizeof(lsm6dso_state),
                              &lsm6dso_activity_sensor_api,
                              &lsm6dso_sensor_instance_api);
}

#endif //!LSM6DSO_ESP_ACTIVITY

