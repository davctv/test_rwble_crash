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
#include <dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_spiffs.h"



#define TAG "example"

#define PARTITION_LABEL		"users"
//Incresase this number and "boom"...program stack overflow..
#define NUMBER_OF_UT_FILES 100

DIR *mydir;
struct dirent* entry;
uint16_t counter = 0;

void app_main()
{

	esp_log_level_set(TAG, ESP_LOG_DEBUG);
	esp_log_level_set("SPIFFS", ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "Initializing SPIFFS system");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/U",
      .partition_label = PARTITION_LABEL,
      .max_files = 2,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%d)", ret);
        }
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(PARTITION_LABEL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%d)", ret);
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }


    ret = esp_spiffs_format(PARTITION_LABEL);

    ESP_LOGI(TAG, "Partition formatted:%d", ret);

    for (uint16_t i = 1; i <= NUMBER_OF_UT_FILES; i++)
    {
    	FILE *fp;
    	char string[100];

    	sprintf(string, "/U/UT/foo%03d", i);


    	fp = fopen(string, "w");

    	memset(string, 0x0, sizeof(string));
    	sprintf(string,"This is the content of foo%03d", i);

    	fwrite(string, 1, strlen(string), fp);

    	fclose(fp);

    	ESP_LOGD(TAG, "creating file /U/UT/file%03d", i);
    }


	ESP_LOGI(TAG, "creating file in /U/UT: DONE");

	vTaskDelay(1000 / portTICK_RATE_MS);

	fclose(fopen("/U/UE/bar", "w"));

	ESP_LOGI(TAG, "creating file in /U/UE/bar: DONE");

	vTaskDelay(1000 / portTICK_RATE_MS);



	ESP_LOGI(TAG, "List files in \"folder\" /U");
	ESP_LOGI(TAG, "freeHeap:%d; StackWaterMark:%d", heap_caps_get_free_size(MALLOC_CAP_8BIT), uxTaskGetStackHighWaterMark(NULL));

	mydir = opendir("/U");

	while ((entry = readdir(mydir)))
	{

		ESP_LOGD(TAG, "file list /U:%s", entry->d_name);
		counter++;
	}

	closedir(mydir);

	ESP_LOGI(TAG, "List files in \"folder\" /U DONE: num file %d", counter);





	vTaskDelay(3000 / portTICK_RATE_MS);


	ESP_LOGI(TAG, "List files in \"folder\" /U/UT");
	ESP_LOGI(TAG, "freeHeap:%d; StackWaterMark:%d", heap_caps_get_free_size(MALLOC_CAP_8BIT), uxTaskGetStackHighWaterMark(NULL));


	mydir = opendir("/U/UT");

	counter = 0;

	while ((entry = readdir(mydir)))
	{
		ESP_LOGD(TAG, "file list /U/UT:%s", entry->d_name);
		counter++;
	}

	closedir(mydir);

	ESP_LOGI(TAG, "List files in \"folder\" /U/UT DONE: num file %d", counter);

	vTaskDelay(3000 / portTICK_RATE_MS);





	ESP_LOGI(TAG, "List files in \"folder\" /U/UE");
	ESP_LOGI(TAG, "freeHeap:%d; StackWaterMark:%d", heap_caps_get_free_size(MALLOC_CAP_8BIT), uxTaskGetStackHighWaterMark(NULL));

	mydir = opendir("/U/UE");

	counter = 0;
	while ((entry = readdir(mydir)))
	{

		ESP_LOGD(TAG, "file list /U/UE:%s", entry->d_name);
		counter++;
	}

	ESP_LOGI(TAG, "List files in \"folder\" /U/UE DONE: num file %d", counter);

	closedir(mydir);

	while(1)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
	}


}
