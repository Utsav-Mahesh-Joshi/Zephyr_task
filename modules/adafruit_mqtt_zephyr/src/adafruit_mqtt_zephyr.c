/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Core implementation of Adafruit MQTT Zephyr adapter.
 * Handles initialization, connection, and disconnection.
 */

#include "Adafruit_MQTT_Zephyr.h"
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <string.h>
#include <zephyr/logging/log.h>
//#include <zephyr/random/random.h>
#define MQTT_RX_BUF_SIZE 512
#define MQTT_TX_BUF_SIZE 512

LOG_MODULE_REGISTER(adafruit_mqtt_zephyr);
static struct sockaddr_storage broker_addr;
static bool dns_resolved = false;


static uint8_t rx_buffer[MQTT_RX_BUF_SIZE];
static uint8_t tx_buffer[MQTT_TX_BUF_SIZE];

static void mqtt_event_cb(struct mqtt_client *client,
			  const struct mqtt_evt *evt)
{
	struct Adafruit_MQTT_Z_Client *zclient =
		CONTAINER_OF(client, struct Adafruit_MQTT_Z_Client, client);

	switch (evt->type) {
		case MQTT_EVT_CONNACK:
			LOG_INF("CONNACK result=%d return_code=%d",
					evt->result,
					evt->param.connack.return_code);
			if (evt->result == 0 &&
					evt->param.connack.return_code == MQTT_CONNECTION_ACCEPTED) {
				zclient->connected = true;
			} else {
				LOG_ERR("Connection refused (%d)", evt->param.connack.return_code);
			}
			break;

		case MQTT_EVT_DISCONNECT:
			LOG_WRN("MQTT disconnected,result=%d", evt->result);
			zclient->connected = false;
			break;

		case MQTT_EVT_PUBLISH:
			LOG_INF("Incoming message on topic: %.*s",
					evt->param.publish.message.topic.topic.size,
					evt->param.publish.message.topic.topic.utf8);
			break;

		case MQTT_EVT_PUBACK:
		case MQTT_EVT_PUBREC:
		case MQTT_EVT_PUBREL:
		case MQTT_EVT_PUBCOMP:
		case MQTT_EVT_SUBACK:
		default:
			break;
	}
}

/* ------------------------------------------------------------------------- */
/* Connection Setup                                                          */
/* ------------------------------------------------------------------------- */

//int adafruit_mqtt_zephyr_init(struct Adafruit_MQTT_Z_Client *zclient,
//			      const char *host, uint16_t port)
//{
//	memset(zclient, 0, sizeof(*zclient));
//
//	struct addrinfo *res;
///	struct addrinfo hints = {
//		.ai_family = AF_UNSPEC,
//		.ai_socktype = SOCK_STREAM,
//	};
//	//int rc = getaddrinfo(host, NULL, &hints, &res);
//	int rc =dns_get_addr_info(host, DNS_QUERY_TYPE_A, NULL, dns_resolve_callback, (void *)host, SYS_FOREVER_MS);
///	if (rc != 0) {
//		LOG_ERR("DNS lookup failed (%d)", rc);
//		return -EHOSTUNREACH;
//	}
//
//	memcpy(&zclient->broker, res->ai_addr, res->ai_addrlen);
//	freeaddrinfo(res);
//
//	if (zclient->broker.ss_family == AF_INET) {
//		((struct sockaddr_in *)&zclient->broker)->sin_port = htons(port);
//	} else {
//		((struct sockaddr_in6 *)&zclient->broker)->sin6_port = htons(port);
//	}
//
///	mqtt_client_init(&zclient->client);
//	zclient->client.broker = &zclient->broker;
//	zclient->client.evt_cb = mqtt_event_cb;
///	zclient->client.client_id.utf8 = "zephyr_client";
//	zclient->client.client_id.size = strlen("zephyr_client");
//	zclient->client.protocol_version = MQTT_VERSION_3_1_1;
//	zclient->client.rx_buf = rx_buffer;
//	zclient->client.rx_buf_size = sizeof(rx_buffer);
//	zclient->client.tx_buf = tx_buffer;
//	zclient->client.tx_buf_size = sizeof(tx_buffer);
//	zclient->client.transport.type = MQTT_TRANSPORT_NON_SECURE;
//	zclient->keepalive = 60;
//	zclient->connected = false;
//
//	return 0;
//}
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dns_resolve.h>


int adafruit_mqtt_zephyr_init(struct Adafruit_MQTT_Z_Client *zclient,
                              const char *host, uint16_t port,
                              const char *aio_username,
                              const char *aio_key)
{
    memset(zclient, 0, sizeof(*zclient));

    /* Ensure Wi-Fi + DHCP are ready */
    struct net_if *iface = net_if_get_default();

    while (!net_if_is_up(iface)) {
        LOG_WRN("Waiting for interface to come up...");
        k_sleep(K_MSEC(200));
    }

    while (!net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED)) {
        LOG_WRN("Waiting for DHCP IPv4 address...");
        k_sleep(K_MSEC(200));
    }

    /* Optional: small delay for DNS stability */
    k_sleep(K_SECONDS(1));

    /* --- DNS Lookup using zsock --- */
    LOG_INF("Resolving host: %s", host);

    struct zsock_addrinfo hints = {
        .ai_family = AF_INET,      /* IPv4 only */
        .ai_socktype = SOCK_STREAM,
    };
    struct zsock_addrinfo *res = NULL;

    int err = zsock_getaddrinfo(host, NULL, &hints, &res);
    if (err) {
        LOG_ERR("getaddrinfo() failed, err %d", err);
        return -EHOSTUNREACH;
    }

    memcpy(&broker_addr, res->ai_addr, res->ai_addrlen);
    zsock_freeaddrinfo(res);

    if (broker_addr.ss_family == AF_INET) {
        ((struct sockaddr_in *)&broker_addr)->sin_port = htons(port);
        char ip[NET_IPV4_ADDR_LEN];
        net_addr_ntop(AF_INET, &((struct sockaddr_in *)&broker_addr)->sin_addr, ip, sizeof(ip));
        LOG_INF("Resolved IPv4: %s", ip);
    } else {
        LOG_ERR("Unsupported address family");
        return -EAFNOSUPPORT;
    }

    /* --- MQTT Client Setup --- */
    mqtt_client_init(&zclient->client);

    zclient->broker = broker_addr;
    zclient->client.broker = &zclient->broker;
    zclient->client.evt_cb = mqtt_event_cb;

    snprintf(zclient->client_id, sizeof(zclient->client_id), "zephyr_%u", (unsigned)k_cycle_get_32());
    zclient->client.client_id.utf8 = zclient->client_id;
    zclient->client.client_id.size = strlen(zclient->client_id);

    //zclient->client.client_id.utf8 = NULL;
    //zclient->client.client_id.size = NULL;

    static uint8_t user_buf[32];
    static uint8_t key_buf[64];

    strcpy((char *)user_buf, aio_username);
    strcpy((char *)key_buf, aio_key);

    zclient->uname.utf8 = user_buf;
    zclient->uname.size = strlen((char *)user_buf);
    zclient->pass.utf8  = key_buf;
    zclient->pass.size  = strlen((char *)key_buf);

    //client->uname.utf8 = (uint8_t *)aio_username;
    //client->uname.size = strlen(aio_username);
    //client->pass.utf8  = (uint8_t *)aio_key;
    //client->pass.size  = strlen(aio_key);

    zclient->client.user_name = &zclient->uname;
    zclient->client.password  = &zclient->pass;


    zclient->client.protocol_version = MQTT_VERSION_3_1_1;
    zclient->client.transport.type = MQTT_TRANSPORT_NON_SECURE;

    zclient->client.rx_buf = rx_buffer;
    zclient->client.rx_buf_size = sizeof(rx_buffer);
    zclient->client.tx_buf = tx_buffer;
    zclient->client.tx_buf_size = sizeof(tx_buffer);

    //zclient->keepalive = 60;
    zclient->client.clean_session=1;
    zclient->client.keepalive = 60;         // <-- IMPORTANT

    zclient->connected = false;

    LOG_INF("MQTT client initialized for Adafruit IO user '%s'", aio_username);
    return 0;
}


int adafruit_mqtt_zephyr_connect(struct Adafruit_MQTT_Z_Client *zclient)
{
	int rc = mqtt_connect(&zclient->client);
	if (rc != 0) {
		LOG_ERR("mqtt_connect failed %d", rc);
		return rc;
	}
	LOG_INF("mqtt_connect rc =%d",rc);

	/* Wait until connection acknowledged */
	int64_t start = k_uptime_get();
	while (!zclient->connected && k_uptime_get() - start < 5000) {
		mqtt_input(&zclient->client);
		mqtt_live(&zclient->client);
		k_msleep(100);
	}
	return zclient->connected ? 0 : -ETIMEDOUT;
}

void adafruit_mqtt_zephyr_disconnect(struct Adafruit_MQTT_Z_Client *zclient)
{
	if (zclient->connected) {
		mqtt_disconnect(&zclient->client,NULL);
		//mqtt_disconnect(&zclient->client);
		zclient->connected = false;
	}
}

