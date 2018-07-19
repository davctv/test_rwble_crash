/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <netdb.h>
#include <sys/socket.h>



#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "rom/rtc.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
//#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/ledc.h"
#include "driver/rmt.h"

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"



#define TAG "example"


void app_main()
{

	// Initialize NVS.
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
		// OTA app partition table has a smaller NVS partition size than the non-OTA
		// partition table. This size mismatch may cause NVS initialization to fail.
		// If this happens, we erase NVS partition and initialize NVS again.
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}


    ESP_LOGI(TAG, "Initializing SPIFFS system");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/D",
      .partition_label = NULL,
      .max_files = 2,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret1 = esp_vfs_spiffs_register(&conf);

    if (ret1 != ESP_OK) {
        if (ret1 == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret1 == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%d)", ret1);
        }
    }

    size_t total = 0, used = 0;
    ret1 = esp_spiffs_info(NULL, &total, &used);
    if (ret1 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%d)", ret1);
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }


    for (uint16_t i = 0; i < 1000; i++)
    {
    	FILE *fp;
    	char string[100];

    	sprintf(string, "/D/UT/file%03d", i);


    	fp = fopen(string, "w");

    	memset(string, 0x0, sizeof(string));
    	sprintf(string,"This is the content of file%03d", i);

    	fwrite(string, 1, strlen(string), fp);

    	fclose(fp);

    	ESP_LOGD(TAG, "creating file:%03d", i);
    }


	ESP_LOGD(TAG, "creating file in /D/UT: DONE");


	vTaskDelay(1000 / portTICK_RATE_MS);


	ESP_LOGD(TAG, "List files in \"folder\" /D");

	DIR *mydir;
	struct dirent* entry;

	mydir = opendir("/D");


	while ((entry = readdir(mydir)))
	{

		ESP_LOGD(TAG, "file list UT:%s", entry->d_name);

	}

	closedir(mydir);


	vTaskDelay(3000 / portTICK_RATE_MS);


	ESP_LOGD(TAG, "List files in \"folder\" /D/UT");



	mydir = opendir("/D/UT");


	while ((entry = readdir(mydir)))
	{

		ESP_LOGD(TAG, "file list UT:%s", entry->d_name);

	}

	closedir(mydir);

	vTaskDelay(3000 / portTICK_RATE_MS);

	ESP_LOGD(TAG, "List files in \"folder\" /D/UE");



	mydir = opendir("/D/UE");


	while ((entry = readdir(mydir)))
	{

		ESP_LOGD(TAG, "file list UE:%s", entry->d_name);

	}

	closedir(mydir);


}
