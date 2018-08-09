/*
 * glob_inc.h
 *
 *  Created on: 22/mag/2015
 *      Author: sales
 */

#ifndef GLOB_INC_H_
#define GLOB_INC_H_

#define TAG_BLE			"ble"
#define TAG_WIFI		"WiFi"
#define TAG_PING		"ping"
#define TAG_PING_VERB	"ping_vb"
#define TAG_GATTS		"gatts"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <netdb.h>
#include <sys/socket.h>

//FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

//System and WIFI
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

//LOG
#include "esp_log.h"

//OTA
#include "esp_ota_ops.h"

//
#include "rom/rtc.h"
#include "rom/crc.h"


//NVS flash
#include "nvs.h"
#include "nvs_flash.h"

//Filesystems
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

//Peripheral
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/rmt.h"

//Internet
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "ping.h"
#include "esp_ping.h"

//BLE
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"

//ESP-NOW
#include "esp_now.h"



// Include header file


#include "wifi.h"
#include "ble.h"





#endif /* GLOB_INC_H_ */
