#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := test_spiffs_crash
EXCLUDE_COMPONENTS := openssl aws_iot libsodium json jsmn console mdns esp-tls aes esp_http_client coap fatfs

include $(IDF_PATH)/make/project.mk

