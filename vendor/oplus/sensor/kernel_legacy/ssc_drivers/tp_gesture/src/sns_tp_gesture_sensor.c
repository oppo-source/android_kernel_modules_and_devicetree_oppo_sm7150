/************************************************************************************
** Copyright (C), 2008-2018, OPPO Mobile Comm Corp., Ltd
** OPLUS_FEATURE_SENSOR_ALGORITHM
** File: sns_xxx.c
**
** Description:
**      Definitions for free fall detect algorithem .
**
** Version: 1.0
** Date created: 2018/03/09,20:27
**
** --------------------------- Revision History: ------------------------------------
* <version>        <date>        <author>               <desc>
**************************************************************************************/

#include "sns_tp_gesture_sensor.h"
#include "sns_tp_gesture_sensor_instance.h"

static void tp_gesture_publish_attributes(sns_sensor *const this)
{
    {
        char const name[] = "tp_gesture";
        sns_std_attr_value_data value = sns_std_attr_value_data_init_default;

        value.str.funcs.encode = pb_encode_string_cb;
        value.str.arg = &((pb_buffer_arg) {
            .buf = name, .buf_len = sizeof(name)
        });
        sns_publish_attribute(this, SNS_STD_SENSOR_ATTRID_NAME, &value, 1, false);
    }
    {
        char const type[] = "tp_gesture";
        sns_std_attr_value_data value = sns_std_attr_value_data_init_default;

        value.str.funcs.encode = pb_encode_string_cb;
        value.str.arg = &((pb_buffer_arg) {
            .buf = type, .buf_len = sizeof(type)
        });
        sns_publish_attribute(this, SNS_STD_SENSOR_ATTRID_TYPE, &value, 1, false);
    }
    {
        char const vendor[] = "oppo";
        sns_std_attr_value_data value = sns_std_attr_value_data_init_default;

        value.str.funcs.encode = pb_encode_string_cb;
        value.str.arg = &((pb_buffer_arg) {
            .buf = vendor, .buf_len = sizeof(vendor)
        });
        sns_publish_attribute(this, SNS_STD_SENSOR_ATTRID_VENDOR, &value, 1, false);
    }
    {
        sns_std_attr_value_data values[] = {SNS_ATTR, SNS_ATTR, SNS_ATTR};
        values[0].has_flt = true;
        values[0].flt = 10.0;
        values[1].has_flt = true;
        values[1].flt = 50.0;
        values[2].has_flt = true;
        values[2].flt = 100.0;
        sns_publish_attribute(this, SNS_STD_SENSOR_ATTRID_RATES, values, ARR_SIZE(values), false);
    }
    {
        sns_std_attr_value_data values[] = {SNS_ATTR};
        char const proto1[] = "sns_std_sensor.proto";

        values[0].str.funcs.encode = pb_encode_string_cb;
        values[0].str.arg = &((pb_buffer_arg) {
            .buf = proto1, .buf_len = sizeof(proto1)
        });
        sns_publish_attribute(this, SNS_STD_SENSOR_ATTRID_API, values, ARR_SIZE(values), false);
    }
    {
        sns_std_attr_value_data value = sns_std_attr_value_data_init_default;

        value.has_sint = true;
        value.sint = 1;
        sns_publish_attribute(this, SNS_STD_SENSOR_ATTRID_VERSION, &value, 1, true);
    }
    {
        sns_std_attr_value_data value = sns_std_attr_value_data_init_default;

        value.has_sint = true;
        value.sint  = SNS_STD_SENSOR_STREAM_TYPE_ON_CHANGE;
        sns_publish_attribute(this, SNS_STD_SENSOR_ATTRID_STREAM_TYPE, &value, 1, true);
    }
}

static void tp_gesture_publish_available(sns_sensor *const this)
{
    sns_std_attr_value_data value = sns_std_attr_value_data_init_default;

    TP_GESTURE_LOG_0("tp_gesture_publish_available");

    value.has_boolean = true;
    value.boolean = true;
    sns_publish_attribute(this, SNS_STD_SENSOR_ATTRID_AVAILABLE, &value, 1, true);
}

sns_rc sns_tp_gesture_init(sns_sensor *const this)
{
    sns_tp_gesture_sensor_state *state = (sns_tp_gesture_sensor_state *)this->state->state;
    struct sns_service_manager *smgr = this->cb->get_service_manager(this);
    float data[2] = {0};

    state->diag_service = (sns_diag_service *)smgr->get_service(smgr, SNS_DIAG_SERVICE);

    // determine encoded output event size
    state->config.encoded_data_event_len = pb_get_encoded_size_sensor_stream_event(data, 2);

    SNS_SUID_LOOKUP_INIT(state->suid_lookup_data, NULL);
    sns_suid_lookup_add(this, &state->suid_lookup_data, "resampler");
    sns_suid_lookup_add(this, &state->suid_lookup_data, "accel");
    sns_suid_lookup_add(this, &state->suid_lookup_data, "gyro");

    tp_gesture_publish_attributes(this);
    TP_GESTURE_LOG_0("tp_gesture init success and attributes published");

    return SNS_RC_SUCCESS;
}

sns_rc sns_tp_gesture_deinit(sns_sensor *const this)
{
    UNUSED_VAR(this);
    TP_GESTURE_LOG_0("sns_tp_gesture_deinit");
    return SNS_RC_SUCCESS;
}

sns_rc sns_tp_gesture_notify_event(sns_sensor *const this)
{
    sns_tp_gesture_sensor_state *state = (sns_tp_gesture_sensor_state *)this->state->state;

    TP_GESTURE_LOG_0("sns_tp_gesture_notify_event");

    sns_suid_lookup_handle(this, &state->suid_lookup_data);

    if (sns_suid_lookup_complete(&state->suid_lookup_data)) {
        tp_gesture_publish_available(this);

        sns_suid_lookup_deinit(this, &state->suid_lookup_data);
    }

    return SNS_RC_SUCCESS;
}

sns_sensor_instance *sns_tp_gesture_set_client_request(sns_sensor *const this,
    sns_request const *exist_request, sns_request const *new_request, bool remove)
{
    sns_sensor_instance *ret_inst = sns_sensor_util_get_shared_instance(this);

    TP_GESTURE_LOG_2("tp_gesture_set_client_request: exist_request = 0x%x, new_request = 0x%x",
        exist_request, new_request);

    if (remove) {
        if (NULL != ret_inst) {
            ret_inst->cb->remove_client_request(ret_inst, exist_request);
        }
    } else if ((NULL != new_request) && (SNS_STD_MSGID_SNS_STD_FLUSH_REQ == new_request->message_id)
        && (NULL != ret_inst)) {
        TP_GESTURE_LOG_0("Flush client req");

        this->instance_api->set_client_config(ret_inst, new_request);
    } else if ((NULL != new_request)
        && (SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG == new_request->message_id)) {
        if (NULL == ret_inst) {
            // If this is a request from a new client
            TP_GESTURE_LOG_0("create_instance");

            ret_inst = this->cb->create_instance(this, (sizeof(sns_tp_gesture_inst_state)));
        }

        if ((NULL != ret_inst) && (NULL == exist_request)) {
            TP_GESTURE_LOG_0("set_client_config");

            ret_inst->cb->add_client_request(ret_inst, new_request);
            this->instance_api->set_client_config(ret_inst, new_request);
        }
    }

    if (NULL != ret_inst
        && NULL == ret_inst->cb->get_client_request(ret_inst, this->sensor_api->get_sensor_uid(this),
            true)) {
        TP_GESTURE_LOG_0("remove_instance");

        this->cb->remove_instance(ret_inst);
    }

    return ret_inst;
}

