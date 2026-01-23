/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Adafruit MQTT Zephyr adapter – topic & utility helpers
 */

#include "Adafruit_MQTT_Zephyr.h"
#include <string.h>
LOG_MODULE_REGISTER(adafruit_mqtt_topics, CONFIG_ADAFRUIT_MQTT_LOG_LEVEL);

/* ------------------------------------------------------------------------- */
/* Helper: construct full topic paths for feeds                              */
/* ------------------------------------------------------------------------- */

/**
 * Build an Adafruit IO–style topic string: "<user>/feeds/<feedname>"
 *
 * @param user     Adafruit IO username (e.g. "uj742")
 * @param feedname Feed name (e.g. "temperature")
 * @param out      Output buffer
 * @param out_len  Size of output buffer
 * @return 0 on success, -ENOMEM if buffer too small
 */
int adafruit_mqtt_zephyr_build_feed(const char *user,
				    const char *feedname,
				    char *out,
				    size_t out_len)
{
	size_t needed = strlen(user) + strlen(feedname) + strlen("/feeds/") + 1;
	if (needed > out_len)
		return -ENOMEM;

	strcpy(out, user);
	strcat(out, "/feeds/");
//	strcpy(out, "feeds/");
	strcat(out, feedname);
	return 0;
}

/* ------------------------------------------------------------------------- */
/* Helper: parse received topics                                             */
/* ------------------------------------------------------------------------- */

/**
 * Parse incoming topic into username and feed name components.
 * Works only for canonical "<user>/feeds/<feed>" format.
 *
 * @param topic Input topic string
 * @param user_out Buffer for username
 * @param feed_out Buffer for feed name
 * @param maxlen Max buffer length for each
 * @return 0 on success, -EINVAL if format not matched
 */
int adafruit_mqtt_zephyr_parse_feed(const char *topic,
				    char *user_out,
				    char *feed_out,
				    size_t maxlen)
{
	const char *feeds = strstr(topic, "/feeds/");
	if (!feeds)
		return -EINVAL;

	size_t user_len = feeds - topic;
	size_t feed_len = strlen(feeds + 7); /* skip "/feeds/" */

	if (user_len >= maxlen || feed_len >= maxlen)
		return -ENOMEM;

	strncpy(user_out, topic, user_len);
	user_out[user_len] = '\0';
	strcpy(feed_out, feeds + 7);

	return 0;
}

/* ------------------------------------------------------------------------- */
/* Optional convenience logging wrapper                                     */
/* ------------------------------------------------------------------------- */

void adafruit_mqtt_zephyr_log_state(struct Adafruit_MQTT_Z_Client *zclient)
{
	LOG_INF("MQTT state: connected=%d keepalive=%u",
		zclient->connected, zclient->keepalive);
}

