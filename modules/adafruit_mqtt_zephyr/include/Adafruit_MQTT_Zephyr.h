/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Adafruit MQTT Library – Zephyr Adapter
 * --------------------------------------
 * This header provides an Arduino-style API façade over Zephyr’s native
 * <zephyr/net/mqtt.h> client.  It allows existing Adafruit-MQTT sketches
 * to compile and run inside Zephyr with minimal source changes.
 */

#ifndef ADAFRUIT_MQTT_ZEPHYR_H_
#define ADAFRUIT_MQTT_ZEPHYR_H_

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/logging/log.h>


/* ------------------------------------------------------------------------- */
/* Basic data types matching Adafruit MQTT concepts                          */
/* ------------------------------------------------------------------------- */

struct Adafruit_MQTT_Z_Client
{
	const struct device *net_iface;
	struct mqtt_client client;
	struct sockaddr_storage broker;

	struct mqtt_publish_param pub_param;
	struct mqtt_subscription_list *sub_list;

	bool connected;
	uint16_t keepalive;
	struct mqtt_utf8 uname;
	struct mqtt_utf8 pass;
	char client_id[32];
};

/* ------------------------------------------------------------------------- */
/* Initialization and Connection                                             */
/* ------------------------------------------------------------------------- */

/**
 * Initialize a Zephyr MQTT client structure and underlying socket.
 *
 * @param zclient Pointer to Adafruit MQTT Zephyr client
 * @param host    Broker hostname or IPv4/IPv6 literal
 * @param port    Broker port (e.g., 1883)
 * @return 0 on success, negative errno otherwise
 */
int adafruit_mqtt_zephyr_init(struct Adafruit_MQTT_Z_Client *zclient,
			      const char *host, uint16_t port,const char *aio_username,
			      const char *aio_key);

/**
 * Connect to broker (blocking until success or timeout).
 */
int adafruit_mqtt_zephyr_connect(struct Adafruit_MQTT_Z_Client *zclient);

/**
 * Disconnect from broker and close socket.
 */
void adafruit_mqtt_zephyr_disconnect(struct Adafruit_MQTT_Z_Client *zclient);

/* ------------------------------------------------------------------------- */
/* Publish / Subscribe                                                       */
/* ------------------------------------------------------------------------- */

/**
 * Publish a message to a topic.
 *
 * @param topic   UTF-8 topic string
 * @param payload Pointer to data buffer
 * @param len     Length of data
 * @param qos     QoS level (0 or 1)
 * @param retain  Retain flag
 */
int adafruit_mqtt_zephyr_publish(struct Adafruit_MQTT_Z_Client *zclient,
				 const char *topic,
				 const void *payload,
				 size_t len,
				 uint8_t qos,
				 bool retain);

/**
 * Subscribe to a topic.
 *
 * @param topic UTF-8 topic string
 * @param qos   QoS level
 */
int adafruit_mqtt_zephyr_subscribe(struct Adafruit_MQTT_Z_Client *zclient,
				   const char *topic, uint8_t qos);

/**
 * Poll and process incoming packets; call user callback when messages arrive.
 * Should be invoked periodically from the main loop or a dedicated thread.
 */
int adafruit_mqtt_zephyr_yield(struct Adafruit_MQTT_Z_Client *zclient,
			       int timeout_ms);
int adafruit_mqtt_zephyr_build_feed(const char *user,
				    const char *feedname,
				    char *out,
				    size_t out_len);


/* ------------------------------------------------------------------------- */
/* Utility                                                                   */
/* ------------------------------------------------------------------------- */

/**
 * Returns true if currently connected to broker.
 */
static inline bool adafruit_mqtt_zephyr_connected(
	const struct Adafruit_MQTT_Z_Client *zclient)
{
	return zclient->connected;
}

#endif /* ADAFRUIT_MQTT_ZEPHYR_H_ */

