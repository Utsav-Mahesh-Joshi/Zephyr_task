/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Adafruit MQTT Zephyr adapter – publish, subscribe, and yield logic
 */

#include "Adafruit_MQTT_Zephyr.h"
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adafruit_mqtt_client, CONFIG_ADAFRUIT_MQTT_LOG_LEVEL);

/* ------------------------------------------------------------------------- */
/* Publish                                                                   */
/* ------------------------------------------------------------------------- */
int adafruit_mqtt_zephyr_publish(struct Adafruit_MQTT_Z_Client *zclient,
				 const char *topic,
				 const void *payload,
				 size_t len,
				 uint8_t qos,
				 bool retain)
{
	uint16_t msg_id=0;
	if (!zclient->connected)
		return -ENOTCONN;

	struct mqtt_publish_param param;
	memset(&param, 0, sizeof(param));

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = (uint8_t *)topic;
	param.message.topic.topic.size = strlen(topic);
	param.message.payload.data = (void *)payload;
	param.message.payload.len = len;
	param.message_id = ++msg_id;
	param.dup_flag = 0;
	param.retain_flag = retain;

	int rc = mqtt_publish(&zclient->client, &param);
	if (rc)
		LOG_ERR("Publish failed: %d", rc);
	else
		LOG_DBG("Published to %s", topic);
	return rc;
}

/* ------------------------------------------------------------------------- */
/* Subscribe                                                                 */
/* ------------------------------------------------------------------------- */
int adafruit_mqtt_zephyr_subscribe(struct Adafruit_MQTT_Z_Client *zclient,
				   const char *topic, uint8_t qos)
{
	static uint16_t msg_id=0; 
	if (!zclient->connected)
		return -ENOTCONN;

	struct mqtt_topic mqtt_topic = {
		.topic.utf8 = (uint8_t *)topic,
		.topic.size = strlen(topic),
		.qos = qos,
	};
	struct mqtt_subscription_list sub_list = {
		.list = &mqtt_topic,
		.list_count = 1U,
	//	.message_id = sys_rand32_get(),
		.message_id = ++msg_id,
	};

	int rc = mqtt_subscribe(&zclient->client, &sub_list);
	if (rc)
		LOG_ERR("Subscribe failed: %d", rc);
	else
		LOG_INF("Subscribed to %s", topic);
	return rc;
}

/* ------------------------------------------------------------------------- */
/* Yield / Poll                                                              */
/* ------------------------------------------------------------------------- */
int adafruit_mqtt_zephyr_yield(struct Adafruit_MQTT_Z_Client *zclient,
			       int timeout_ms)
{
	if (!zclient->connected)
		return -ENOTCONN;

	int64_t start = k_uptime_get();
	while (k_uptime_get() - start < timeout_ms) {
		int rc = mqtt_input(&zclient->client);
		if (rc < 0 && rc != -EAGAIN) {
			LOG_ERR("mqtt_input error %d", rc);
			return rc;
		}
		mqtt_live(&zclient->client);
		k_msleep(100);
	}
	return 0;
}

