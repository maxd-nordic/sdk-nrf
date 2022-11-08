/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <net/wifi_mgmt_ext.h>
#include <net/wifi_credentials.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

#include "includes.h"
#include "common.h"
#include "common/defs.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"

extern struct k_sem wpa_supplicant_ready_sem;
extern struct k_mutex wpa_supplicant_mutex;
extern struct wpa_global *global;


LOG_MODULE_REGISTER(wifi_mgmt_ext, CONFIG_WIFI_MGMT_EXT_LOG_LEVEL);

/* helper functions {*/

static inline struct wpa_supplicant * get_wpa_s_handle(const struct device *dev)
{
	struct wpa_supplicant *wpa_s = NULL;
	int ret = k_sem_take(&wpa_supplicant_ready_sem, K_SECONDS(2));

	if (ret) {
		LOG_ERR("%s: WPA supplicant not ready: %d", __func__, ret);
		return NULL;
	}

	k_sem_give(&wpa_supplicant_ready_sem);

	wpa_s = wpa_supplicant_get_iface(global, dev->name);
	if (!wpa_s) {
		LOG_ERR("%s: Unable to get wpa_s handle for %s\n", __func__, dev->name);
		return NULL;
	}

	return wpa_s;
}

static inline int channel_to_freq(const struct wifi_connect_req_params *params,
					   struct wpa_ssid *ssid)
{
	if (params->channel == WIFI_CHANNEL_ANY) {
		return 0;
	}

	/* We use a global channel list here and also use the widest
	 * op_class for 5GHz channels as there is no user input
	 * for these.
	 */
	int freq  = ieee80211_chan_to_freq(NULL, 81, params->channel);

	if (freq <= 0) {
		freq  = ieee80211_chan_to_freq(NULL, 128, params->channel);
	}

	if (freq <= 0) {
		LOG_ERR("Invalid channel %d", params->channel);
		return -EINVAL;
	}

	ssid->scan_freq = os_zalloc(2 * sizeof(int));
	if (!ssid->scan_freq) {
		return -ENOMEM;
	}
	ssid->scan_freq[0] = freq;
	ssid->scan_freq[1] = 0;

	ssid->freq_list = os_zalloc(2 * sizeof(int));
	if (!ssid->freq_list) {
		os_free(ssid->scan_freq);
		return -ENOMEM;
	}
	ssid->freq_list[0] = freq;
	ssid->freq_list[1] = 0;
	
	return 0;
}

static inline int copy_psk(const struct wifi_connect_req_params *params,
				    struct wpa_ssid *ssid)
{
	if (!params->psk && !params->sae_password) {
		return 0;
	}
	if (params->security == 3) {
		ssid->key_mgmt = WPA_KEY_MGMT_SAE;
		str_clear_free(ssid->sae_password);
		ssid->sae_password = dup_binstr(params->sae_password, params->sae_password_length);

		if (ssid->sae_password == NULL) {
			wpa_printf(MSG_ERROR, "%s:Failed to copy sae_password\n",
					__func__);
			return -ENOMEM;
		}
	} else {
		if (params->security == 2)
			ssid->key_mgmt = WPA_KEY_MGMT_PSK_SHA256;
		else
			ssid->key_mgmt = WPA_KEY_MGMT_PSK;

		str_clear_free(ssid->passphrase);
		ssid->passphrase = dup_binstr(params->psk, params->psk_length);

		if (ssid->passphrase == NULL) {
			wpa_printf(MSG_ERROR, "%s:Failed to copy passphrase\n",
			__func__);
			return -ENOMEM;
		}
	}

	wpa_config_update_psk(ssid);

	return 0;
}

static inline int add_network(struct wpa_supplicant *wpa_s, struct wifi_connect_req_params *params)
{
	int ret = 0;
	struct wpa_ssid *network = wpa_supplicant_add_network(wpa_s);

	if (network == NULL) {
		return -EINVAL;
	}

	network->ssid = os_zalloc(sizeof(u8) * WIFI_SSID_MAX_LEN);

	if (network->ssid == NULL) {
		return -ENOMEM;
	}

	memcpy(network->ssid, params->ssid, params->ssid_length);
	network->ssid_len = params->ssid_length;
	network->disabled = 1;
	network->key_mgmt = WPA_KEY_MGMT_NONE;
	network->ieee80211w = params->mfp;

	ret = channel_to_freq(params, network);
	if (ret) {
		return ret;
	}
	wpa_s->conf->filter_ssids = 1;
	wpa_s->conf->ap_scan = 1;

	ret = copy_psk(params, network);
	if (ret) {
		return ret;
	}
	wpa_supplicant_enable_network(wpa_s, network);
	return 0;
}

/* } */

static int wifi_ext_command(uint32_t mgmt_request, struct net_if *iface,
		       void *data, size_t len)
{
	struct wpa_supplicant *wpa_s = NULL;
	const struct device *dev = net_if_get_device(iface);
	struct wifi_connect_req_params *params = { 0 };
	int ret = 0;

	k_mutex_lock(&wpa_supplicant_mutex, K_FOREVER);

	wpa_s = get_wpa_s_handle(dev);
	if (!wpa_s) {
		goto out;
	}
	switch (mgmt_request)
	{
	case NET_REQUEST_WIFI_ATTACH:
		wpas_request_connection(wpa_s);
		break;
	case NET_REQUEST_WIFI_ADD_NETWORK:
		params = data;
		ret = add_network(wpa_s, params);
		break;
	case NET_REQUEST_WIFI_CLEAR_NETWORKS:
		wpa_supplicant_remove_all_networks(wpa_s);
		break;
	default:
		ret = -EINVAL;
		break;
	}
out:
	k_mutex_unlock(&wpa_supplicant_mutex);
	return ret;
};

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_ATTACH, wifi_ext_command);
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_ADD_NETWORK, wifi_ext_command);
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CLEAR_NETWORKS, wifi_ext_command);
