/*
 * config.cpp
 *
 *  Created on: 26.02.2022
 *      Author: steffen
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

/**
 * ReadConfig: read the config file from SPIFFS file system
 *
 * @returns 1 is successfull, or -1 if an error happend.
 */

int ReadConfig() {
	ESP_LOGI(TAG, "Initializing SPIFFS");
	esp_vfs_spiffs_conf_t conf = { .base_path = "/spiffs", .partition_label = NULL, .max_files = 5, .format_if_mount_failed = false };

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return -1;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(conf.partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}

	// Use POSIX and C standard library functions to work with files.
	// First create a file.
	ESP_LOGI(TAG, "Opening config file");

	// Open renamed file for reading
	ESP_LOGI(TAG, "Reading file");
	FILE *f = fopen("/spiffs/parameter.dat", "r");
	if (f == NULL) {
		ESP_LOGE(TAG, "Failed to open file for reading");
		return -2;
	}
	char line[64];
	while (fgets(line, sizeof(line), f) > NULL) {
		// strip newline
		char *pos = strchr(line, '\n');
		if (pos) {
			*pos = '\0';
		}
		if (strlen(line) > 6) {
			if (strncmp(line, "netip", 5) == 0) {
				strncpy(netip, &line[6], 15);
			} else if (strncmp(line, "netma", 5) == 0) {
				strncpy(netma, &line[6], 15);
			} else if (strncmp(line, "netgw", 5) == 0) {
				strncpy(netgw, &line[6], 15);
			} else if (strncmp(line, "srvip", 5) == 0) {
				strncpy(srvip, &line[6], 15);
			} else if (strncmp(line, "srvpo", 5) == 0) {
				srvpo = atoi(&line[6]);
			} else if (strncmp(line, "boxid", 5) == 0) {
				boxid = atoi(&line[6]);
			} else if (strncmp(line, "subbx", 5) == 0) {
				subbx = atoi(&line[6]);
			} else if (strncmp(line, "dbnam", 5) == 0) {
				strncpy(dbnam, &line[6], 15);
			} else if (strncmp(line, "xxxxx", 5) == 0) {

			}
		}
		ESP_LOGI(TAG, "Read from file: '%s'", line);
	}
	fclose(f);
	// All done, unmount partition and disable SPIFFS
	esp_vfs_spiffs_unregister(conf.partition_label);
	ESP_LOGI(TAG, "SPIFFS unmounted");

	return 1;
}

/**
 * WriteConfig: write the config file to the SPIFFS file system
 *
 * @returns 1 is successfull, or -1 if an error happend.
 */
int WriteConfig(char *buff) {
	ESP_LOGI(TAG, "Initializing SPIFFS");
	esp_vfs_spiffs_conf_t conf = { .base_path = "/spiffs", .partition_label = NULL, .max_files = 5, .format_if_mount_failed = false };

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return -1;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(conf.partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}

	ESP_LOGI(TAG, "Opening config file");
	ESP_LOGI(TAG, "Writing file");
	FILE *f = fopen("/spiffs/parameter.dat", "w");
	if (f == NULL) {
		ESP_LOGE(TAG, "Failed to open file for writing");
		return -2;
	}

	fprintf(f, "%s", buff);

	fclose(f);
	// All done, unmount partition and disable SPIFFS
	esp_vfs_spiffs_unregister(conf.partition_label);
	ESP_LOGI(TAG, "SPIFFS unmounted");

	return 1;
}

/**
 * SaveConfig: saves the config settings from settings screen SPIFFS file system
 *
 * @returns 1 is successfull, or -1 if an error happend.
 */
int SaveConfig(void) { // saves the config from settings screen
	uint8_t ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0;
	char buff[32];
	int i = 0;
	ESP_LOGI(TAG, "Initializing SPIFFS");
	esp_vfs_spiffs_conf_t conf = { .base_path = "/spiffs", .partition_label = NULL, .max_files = 5, .format_if_mount_failed = false };

	esp_err_t ret = esp_vfs_spiffs_register(&conf);
	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return -1;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(conf.partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}

	ESP_LOGI(TAG, "Opening config file");
	FILE *f = fopen("/spiffs/parameter.dat", "w");
	if (f == NULL) {
		ESP_LOGE(TAG, "Failed to open file for writing");
		return -2;
	}

	LCD->iReadValue((char *)"ip1",buff);
	ip1=atoi(buff);
	LCD->iReadValue((char *)"ip2",buff);
	ip2=atoi(buff);
	LCD->iReadValue((char *)"ip3",buff);
	ip3=atoi(buff);
	LCD->iReadValue((char *)"ip4",buff);
	ip4=atoi(buff);
	fprintf(f, "netip=%d.%d.%d.%d\n", ip1, ip2, ip3, ip4);

	LCD->iReadValue((char *)"ma1",buff);
	ip1=atoi(buff);
	LCD->iReadValue((char *)"ma2",buff);
	ip2=atoi(buff);
	LCD->iReadValue((char *)"ma3",buff);
	ip3=atoi(buff);
	LCD->iReadValue((char *)"ma4",buff);
	ip4=atoi(buff);
	fprintf(f, "netma=%d.%d.%d.%d\n", ip1, ip2, ip3, ip4);

	LCD->iReadValue((char *)"gw1",buff);
	ip1=atoi(buff);
	LCD->iReadValue((char *)"gw2",buff);
	ip2=atoi(buff);
	LCD->iReadValue((char *)"gw3",buff);
	ip3=atoi(buff);
	LCD->iReadValue((char *)"gw4",buff);
	ip4=atoi(buff);
	fprintf(f, "netgw=%d.%d.%d.%d\n", ip1, ip2, ip3, ip4);

	LCD->iReadValue((char *)"sip1",buff);
	ip1=atoi(buff);
	LCD->iReadValue((char *)"sip2",buff);
	ip2=atoi(buff);
	LCD->iReadValue((char *)"sip3",buff);
	ip3=atoi(buff);
	LCD->iReadValue((char *)"sip4",buff);
	ip4=atoi(buff);
	fprintf(f, "srvip=%d.%d.%d.%d\n", ip1, ip2, ip3, ip4);

	LCD->iReadValue((char *)"srvpo1",buff);
	i=atoi(buff);
	fprintf(f, "srvpo=%d\n", i);

	LCD->iReadValue((char *)"boxid1",buff);
	i=atoi(buff);
	fprintf(f, "boxid=%d\n", i);

	LCD->iReadValue((char *)"subbx1",buff);
	i=atoi(buff);
	fprintf(f, "subbx=%d\n", i);

	LCD->iReadValue((char *)"dbnam1",buff);
	fprintf(f, "dbnam=%s\n", buff);

	fclose(f);
	// All done, unmount partition and disable SPIFFS
	esp_vfs_spiffs_unregister(conf.partition_label);
	ESP_LOGI(TAG, "SPIFFS unmounted");

	return 0;
}

/**
 * SaveConfig: saves the config settings from settings screen SPIFFS file system
 *
 * @param name, strvals the key/value pair.
 */
void WriteVals(char *name, char *strvals) {
	uint8_t ipp = 0;
	uint32_t ip;
	int i;
	char nm[32];
	ip = esp_ip4addr_aton(strvals);
	for (i = 1; i < 5; i++) {
		ipp = ip & 0xFF;
		sprintf(nm, "%s%d", name, i);
		LCD->iWriteValue(nm, ipp);
		ip = ip >> 8;
	}
}

/**
 * HandleEdit: GUI to modify settings
 *
 * @returns HMI success/error code.
 */
int HandleEdit(void) {
	int sl=1;
	int res;
	while (sl) {
		res=LCD->iReadProt(1);
		printf("HE Res:%d\n",res);fflush(stdout);
		if (res==HMIResultOK) {
			vTaskDelay(1);
			res=LCD->iReadProt(1);
			vTaskDelay(1);
			SaveConfig();
			sl=0;
			LCD->iSendProt(6, (char*) "page 0");
		} else vTaskDelay(50);
	}
	return res;
}

/**
 * EditConfig: GUI to modify settings
 *
 * @returns HMI success/error code.
 */
int EditConfig() {
	LCD->iSendProt(6, (char*) "page 3");
	WriteVals((char *)"ip",netip);
	WriteVals((char *)"gw",netgw);
	WriteVals((char *)"ma",netma);
	WriteVals((char *)"ma",netma);
	WriteVals((char *)"sip",srvip);
	LCD->iWriteValue((char*) "srvpo1", srvpo);
	LCD->iWriteValue((char*) "boxid1", boxid);
	LCD->iWriteValue((char*) "subbx1", subbx);
	LCD->iWriteValue((char*) "dbnam1", dbnam);
	return HandleEdit();
}
