#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include <zephyr/net/net_event.h>
#include "ping.h"
LOG_MODULE_REGISTER(wifi_demo);

/* ---- SET THESE or pass via Kconfig/overlay/env ---- */
#define WIFI_SSID      "Utsav"
#define WIFI_PASSWORD  "utsav12345"
#define WIFI_SECURITY  WIFI_SECURITY_TYPE_PSK  /* WPA2-PSK (AES) */
#define WIFI_TIMEOUT_MS 15000                  /* per attempt */

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;
/* Track state so main thread can wait */
static K_SEM_DEFINE(conn_sem, 0, 1);
static K_SEM_DEFINE(ipv4_sem, 0, 1);

static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint64_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) 
	{
		case NET_EVENT_WIFI_CONNECT_RESULT: 
			const struct wifi_status *st = (const struct wifi_status *) cb->info;
			LOG_INF("WiFi connect result: %d", st ? st->status : -1);
			if(st->status)
			{
				LOG_INF("Connection request failed (%d)", st->status);
			}
			else
			{
				LOG_INF("Connected");
				k_sem_give(&conn_sem);
			}
			break;

		case NET_EVENT_WIFI_DISCONNECT_RESULT:
			const struct wifi_status *status = (const struct wifi_status*)cb->info;
			if(status->status)
			{
				LOG_INF("Disconnect request %d",status->status);
			}
			else
			{
				LOG_INF("WiFi disconnected");
				k_sem_take(&conn_sem,K_NO_WAIT);
			}
			break;

		case NET_EVENT_IPV4_ADDR_ADD:
			int32_t i = 0;

			for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++)
			{

				char buf[NET_IPV4_ADDR_LEN];

				if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type != NET_ADDR_DHCP) 
					continue;

				LOG_INF("IPv4 address: %s",
						net_addr_ntop(AF_INET,
							&iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr,
							buf, sizeof(buf)));
				LOG_INF("Subnet: %s",
						net_addr_ntop(AF_INET,
							&iface->config.ip.ipv4->unicast[i].netmask,
							buf, sizeof(buf)));
				LOG_INF("Router: %s",
						net_addr_ntop(AF_INET,
							&iface->config.ip.ipv4->gw,
							buf, sizeof(buf)));
			}
			LOG_INF("Got IPv4 address");
			k_sem_give(&ipv4_sem);
			break;

		default:
			break;
	}
}

static int do_wifi_scan(struct net_if *iface)
{
    LOG_INF("Starting WiFi scan...");
    int ret = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);
    if (ret) {
        LOG_WRN("wifi scan request failed: %d", ret);
    }
    /* Optional: you can register NET_EVENT_WIFI_SCAN_RESULT / DONE to print results */
    return ret;
}

static int wifi_connect()
{
	//struct net_if *iface = net_if_get_wifi_sta();
	struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params params = {
        .ssid = WIFI_SSID,
        .ssid_length = strlen(WIFI_SSID),
        .psk = WIFI_PASSWORD,
        .psk_length = strlen(WIFI_PASSWORD),
        .security = WIFI_SECURITY,
        .channel = WIFI_CHANNEL_ANY,
        .mfp = WIFI_MFP_OPTIONAL,
	.band=WIFI_FREQ_BAND_2_4_GHZ,
    };

    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
    LOG_INF("wifi connect requested: SSID=\"%s\"", params.ssid);
    if (ret) 
    {
        LOG_ERR("net_mgmt CONNECT failed: %d", ret);
        return ret;
    }
    return 0;
}
void wifi_status()
{

	struct net_if *iface = net_if_get_default();
	//struct net_if *iface =net_if_get_wifi_sta();
	struct wifi_iface_status status = {0};
	if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,sizeof(struct wifi_iface_status)))
   	{
        	LOG_INF("WiFi Status Request Failed");
    	}

	if(status.state >= WIFI_STATE_ASSOCIATED)
	{
		LOG_INF("SSID: %-32s",status.ssid);
		LOG_INF("Band: %s",wifi_band_txt(status.band));
		LOG_INF("Channel: %d",status.channel);
		LOG_INF("Security:%s",wifi_security_txt(status.security));
		LOG_INF("RSSI:%d",status.rssi);
	}

}

void wifi_disconnect()
{
	//struct net_if *iface =net_if_get_wifi_sta();
	struct net_if *iface = net_if_get_default();
	if(net_mgmt(NET_REQUEST_WIFI_DISCONNECT,iface,NULL,0))
	{
		LOG_INF("Wifi Disconnection failed");
	}

}
int main(void)
{
	printk("chik chik \r\n");
	LOG_INF("Wifi Example \r\nBoard:%s",CONFIG_BOARD);
	net_mgmt_init_event_callback(&wifi_cb,wifi_event_handler,
			NET_EVENT_WIFI_CONNECT_RESULT|NET_EVENT_WIFI_DISCONNECT_RESULT);

	net_mgmt_init_event_callback(&ipv4_cb,wifi_event_handler,NET_EVENT_IPV4_ADDR_ADD);
	
	printk("chik chik on net_mgmt\r\n");
	net_mgmt_add_event_callback(&wifi_cb);
	net_mgmt_add_event_callback(&ipv4_cb);

	wifi_connect();

	k_sem_take(&conn_sem,K_FOREVER);

	wifi_status(&ipv4_sem,K_FOREVER);
	ping("8.8.8.8",4);
}

