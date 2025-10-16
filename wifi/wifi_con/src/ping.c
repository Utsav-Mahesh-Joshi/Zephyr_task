/*#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(ping_sock);

#define PING_TIMEOUT_MS	 2000
#define PING_DATA_SIZE	 32
#define PING_INTERVAL_MS 2000

void ping(const char* host,uint8_t count)
{
	int sock;
	struct sockaddr_in addr;

	char send_buf[PING_DATA_SIZE];
	char recv_buf[64];
	int ret;
	int i;

	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=0;

	ret=zsock_inet_pton(AF_INET,host,&addr.sin_addr);
	if(ret <= 0)
	{
		LOG_ERR("Invalid IP address: %s",host);
		return;
	}

	sock=zsock_socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
	if(sock<0)
	{
		LOG_ERR("Unable to create ICMP socket, err=%d",sock);
		return;
	}

	struct timeval timeout={
		.tv_sec=PING_TIMEOUT_MS /1000,
		.tv_usec=(PING_TIMEOUT_MS%1000)*1000
	};

	zsock_setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));

	for(int i=0;i<count;i++)
	{
		memset(send_buf,0,sizeof(send_buf));
		strcpy(send_buf,"zephyr-ping");
		uint32_t start_time = k_uptime_get_32();
		ret=zsock_sendto(sock,send_buf,sizeof(send_buf),0,
				(struct sockaddr*)&addr,sizeof(addr));
		if(ret<0)
		{
			LOG_ERR("Send failed %d ",ret);
		}

		ret=zsock_recv(sock,recv_buf,sizeof(recv_buf),0);
		if(ret)
		{
			LOG_WRN("Request Timeout");
		}
		else
		{
			uint32_t elapsed = k_uptime_get_32()-start_time;
			LOG_INF("Reply from %s : bytes %d time=%ds",host,ret,elapsed);
		}

		k_msleep(PING_INTERVAL_MS);
	}
	zsock_close(sock);



}
*/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
//#include <zephyr/drivers/wifi/eswifi/eswifi.h>
#include "eswifi.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ping_at, LOG_LEVEL_INF);

void eswifi_send_at_ping(const char *ip)
{
    //const struct device *dev = DEVICE_DT_GET_ONE(ism43362);
    //const struct device *dev = DEVICE_DT_GET(wifi0)
    const struct device *dev = DEVICE_DT_GET(DT_ALIAS(eswifi0));
    if (!device_is_ready(dev)) {
        LOG_ERR("esWiFi device not ready");
        return;
    }

    struct eswifi_dev *eswifi = dev->data;
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "T1=%s\r", ip);

    k_mutex_lock(&eswifi->mutex, K_FOREVER);
    int ret = eswifi_request(eswifi, cmd, strlen(cmd), eswifi->buf, sizeof(eswifi->buf));
    snprintf(cmd, sizeof(cmd), "T0\r");
    ret = eswifi_request(eswifi, cmd, strlen(cmd), eswifi->buf, sizeof(eswifi->buf));

    k_mutex_unlock(&eswifi->mutex);

    if (ret > 0) {
        LOG_INF("PING response: %s", eswifi->buf);
    } else {
        LOG_ERR("PING failed, ret=%d", ret);
    }
}
void ping(const char *ip,uint8_t count)
{
	for(uint8_t i=0;i<count;i++)
	{
		eswifi_send_at_ping(ip);
		k_msleep(100);
	}
}
