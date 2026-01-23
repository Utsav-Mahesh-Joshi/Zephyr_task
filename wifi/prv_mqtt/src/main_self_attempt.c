#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/dns_resolve.h>
#include <string.h>
#include <errno.h>

LOG_MODULE_REGISTER(app);

/*********wifi credentials*********/

#define WIFI_SSID 	"Utsav"
#define WIFI_PASSWORD 	"utsav12345"


/***************MQTT***************/

#define MQTT_BROKER_HOST "utsavmqtt.in"
#define MQTT_BROKER_PORT  1883
#define MQTT_CLIENT_ID   "zephyr_client"
#define MQTT_TOPIC	 "test"
#define MQTT_KEEPALIVE	  60

static struct mqtt_client client;
static struct sockaddr_storage broker;
static uint8_t rx_buffer[512];
static uint8_t tx_buffer[512];
static uint8_t payload_buf[256];
static bool    mqtt_connected = false;
static bool wifi_ready=true;
static struct net_mgmt_wifi_callback wifi_cb;

/* ------------ Wi-Fi Events ------------ */
static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint64_t mgmt_event,
                               struct net_if *iface)
{
        ARG_UNUSED(cb);
        ARG_UNUSED(iface);

        switch (mgmt_event) {
        case NET_EVENT_WIFI_CONNECT_RESULT:
                LOG_INF("✅ Wi-Fi connected");
                wifi_ready = true;
                break;

        case NET_EVENT_WIFI_DISCONNECT_RESULT:
                LOG_WRN("⚠️ Wi-Fi disconnected");
                wifi_ready = false;
                break;

        default:
                break;
        }
}
/* ------------ Non-Blocking Wi-Fi Connect ------------ */
static int connect_wifi(void)
{
        struct net_if *iface = net_if_get_default();

        net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler,
                NET_EVENT_WIFI_CONNECT_RESULT |
                NET_EVENT_WIFI_DISCONNECT_RESULT);
        net_mgmt_add_event_callback(&wifi_cb);

        struct wifi_connect_req_params cnx = {
                .ssid        = WIFI_SSID,
                .ssid_length = strlen(WIFI_SSID),
                .psk         = WIFI_PASSWORD,
                .psk_length  = strlen(WIFI_PASSWORD),
                .security    = WIFI_SECURITY_TYPE_PSK,
                .channel     = WIFI_CHANNEL_ANY,
                .timeout     = SYS_FOREVER_MS
        };

        LOG_INF("Connecting to Wi-Fi SSID: %s …", WIFI_SSID);
        int rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &cnx, sizeof(cnx));
        if (rc) {
                LOG_ERR("Wi-Fi connect request failed (%d)", rc);
                return rc;
        }
                        
        return 0; /* Non-blocking — return immediately */
}

/* ------------ DNS Resolve ------------ */
static int broker_resolve(void)
{
        struct zsock_addrinfo hints = {
                .ai_family   = AF_INET,
                .ai_socktype = SOCK_STREAM
        };
        struct zsock_addrinfo *res = NULL;

        int rc = zsock_getaddrinfo(MQTT_BROKER_HOST, NULL, &hints, &res);
        if (rc) {
                LOG_ERR("DNS lookup failed (%d)", rc);
                return rc;
        }

        struct sockaddr_in *broker4 = (struct sockaddr_in *)res->ai_addr;
        broker4->sin_port = htons(MQTT_BROKER_PORT);
        memcpy(&broker, broker4, sizeof(struct sockaddr_in));

        char addr_str[NET_IPV4_ADDR_LEN];
        net_addr_ntop(AF_INET, &broker4->sin_addr, addr_str, sizeof(addr_str));
        LOG_INF("Resolved broker %s:%d", addr_str, MQTT_BROKER_PORT);

        zsock_freeaddrinfo(res);
        return 0;
}



/* ------------ MQTT Event Handler ------------ */
static void mqtt_event_handler(struct mqtt_client *const c,
                               const struct mqtt_evt *evt)
{
        switch (evt->type) {
        case MQTT_EVT_CONNACK:
                if (evt->result == 0) {
                        LOG_INF("MQTT connected");
                        mqtt_connected = true;

                        static struct mqtt_topic sub_topic = {
                                .topic = {
                                        .utf8 = (uint8_t *)MQTT_SUB_TOPIC,
                                        .size = sizeof(MQTT_SUB_TOPIC) - 1
                                },
                                .qos = MQTT_QOS_1_AT_LEAST_ONCE
                        };
                        static struct mqtt_subscription_list sub_list = {
                                .list = &sub_topic,
                                .list_count = 1,
                                .message_id = 1
                        };
                        mqtt_subscribe(c, &sub_list);
                        LOG_INF("Subscribed to %s", MQTT_SUB_TOPIC);
                        /* Optional: publish once immediately after CONNACK */
                        /* mqtt_publish_message(); */
                }
                break;

        case MQTT_EVT_DISCONNECT:
                LOG_WRN("MQTT disconnected");
                mqtt_connected = false;
                break;

        case MQTT_EVT_PINGRESP:
                LOG_INF("PINGRESP");
                break;

        case MQTT_EVT_PUBLISH: {
                const struct mqtt_publish_param *p = &evt->param.publish;
                int len = MIN((size_t)p->message.payload.len, sizeof(payload_buf) - 1);
                int rc = mqtt_read_publish_payload_blocking(c, payload_buf, len);
                if (rc > 0) {
                        payload_buf[rc] = '\0';
                } else {
                        payload_buf[0] = '\0';
                }
                LOG_INF("RX topic=%.*s payload=%s",
                        p->message.topic.topic.size,
                        p->message.topic.topic.utf8,
                        payload_buf);
                break;
        }

        case MQTT_EVT_PUBACK:
                LOG_INF("PUBACK (QoS1)");
                break;

        default:
                break;
        }
}

