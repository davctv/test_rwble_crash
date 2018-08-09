
#include "glob_inc.h"



/***************************************************************************
							Local Definitions
***************************************************************************/
#define WIFI_INTERNETCHECK_TIME	100
#define WIFI_PING_COUNT			5
#define WIFI_PING_TIMEOUT		2000 //2 second (incrementato perch� se BLE � usato contemporaneamente sembra non funzionare)


/***************************************************************************
							Local Variables
***************************************************************************/
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t WIFI_event_group;


bool WIFI_CurrentMode = true;
bool WIFI_isConnected = false;
uint16_t WIFI_ping_check = WIFI_INTERNETCHECK_TIME;
int8_t WIFI_ping_result = 0;
bool WIFI_InternetAvailable = false;



/***************************************************************************
							Program Code
***************************************************************************/




esp_err_t WIFI_event_handler(void *ctx, system_event_t *event)
{
	ESP_LOGD(TAG_WIFI, "event:%d", event->event_id);

    switch(event->event_id)
    {
    case SYSTEM_EVENT_WIFI_READY:
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_WIFI_READY");
    	break;

    case SYSTEM_EVENT_STA_START:
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_STA_START");
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
    {
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_STA_GOT_IP");
    	ESP_LOGD(TAG_WIFI, "Net mask:%s\n", inet_ntoa( event->event_info.got_ip.ip_info.netmask ) );
    	ESP_LOGD(TAG_WIFI, "Gateway:%s\n", inet_ntoa( event->event_info.got_ip.ip_info.gw ) );

    	xEventGroupSetBits(WIFI_event_group, WIFI_CONNECTED_BIT);


    	WIFI_isConnected = true;
    }

        break;

    case SYSTEM_EVENT_STA_LOST_IP:
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_STA_LOST_IP");
    	xEventGroupClearBits(WIFI_event_group, WIFI_CONNECTED_BIT);
    	break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
    {
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_STA_DISCONNECTED:%d,%s", event->event_info.disconnected.reason, event->event_info.disconnected.ssid);

		ESP_LOGD(TAG_WIFI, "TRY to RICONNECT");

		esp_wifi_connect();

    	xEventGroupClearBits(WIFI_event_group, WIFI_CONNECTED_BIT);


    }

        break;

    case SYSTEM_EVENT_STA_CONNECTED:
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_STA_CONNECTED");

        break;

    case SYSTEM_EVENT_AP_START:
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_AP_START");
        break;

    case SYSTEM_EVENT_AP_STOP:
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_AP_STOP");
        break;

    case SYSTEM_EVENT_AP_STACONNECTED:
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_AP_STACONNECTED");
        break;

    case SYSTEM_EVENT_AP_STADISCONNECTED:
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_AP_STADISCONNECTED");
        break;

    case SYSTEM_EVENT_AP_PROBEREQRECVED:
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_AP_PROBEREQRECVED");
        break;

    case SYSTEM_EVENT_GOT_IP6:
    	ESP_LOGD(TAG_WIFI, "SYSTEM_EVENT_AP_STA_GOT_IP6");
        break;

    default:
        break;
    }

    return (ESP_OK);
}



esp_err_t WIFI_pingResults(ping_target_id_t msgType, esp_ping_found * pf)
{

	ESP_LOGD(TAG_PING_VERB, "Resp(mS):%d Timeouts:%d Total Time:%d",pf->resp_time, pf->timeout_count, pf->total_time);

	if (pf->send_count == WIFI_PING_COUNT)
	{
		if (WIFI_ping_result < 0)
		{
			WIFI_InternetAvailable = false;
;
		}
		else
		{
			WIFI_InternetAvailable = true;

		}

		ESP_LOGD(TAG_PING, "avail:%d,%d", WIFI_ping_result, WIFI_InternetAvailable);
	}
	else
	{
		if (pf->resp_time < WIFI_PING_TIMEOUT)
			WIFI_ping_result++;
		else
			WIFI_ping_result--;
	}

	ESP_LOGD(TAG_PING_VERB, "WIFI_ping_result:%d", WIFI_ping_result);

	return (ESP_OK);
}


void WIFI_State_Machine(void)
{



	if (WIFI_ping_check == 0)
	{
		if ((xEventGroupClearBits(WIFI_event_group, 0) & WIFI_CONNECTED_BIT) == WIFI_CONNECTED_BIT)
		{

			uint32_t WIFI_ping_count = WIFI_PING_COUNT;  //how many pings per report
			uint32_t WIFI_ping_timeout = WIFI_PING_TIMEOUT; //mS till we consider it timed out
			uint32_t WIFI_ping_delay = 500; //mS between pings
			ip4_addr_t WIFI_ping_address;

			WIFI_ping_check = WIFI_INTERNETCHECK_TIME;

			inet_aton("212.237.23.42", &WIFI_ping_address);

			esp_ping_set_target(PING_TARGET_IP_ADDRESS_COUNT, &WIFI_ping_count, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_RCV_TIMEO, &WIFI_ping_timeout, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_DELAY_TIME, &WIFI_ping_delay, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_IP_ADDRESS, &WIFI_ping_address, sizeof(uint32_t));
			esp_ping_set_target(PING_TARGET_RES_FN, &WIFI_pingResults, sizeof(WIFI_pingResults));

			ESP_LOGD(TAG_PING, "Pinging IP %s",inet_ntoa(WIFI_ping_address));

			WIFI_ping_result = 0;

			ping_init();
		}

	}
	else
		WIFI_ping_check--;



}


void Init_WIFI_Service(void)
{
	ESP_LOGD(TAG_WIFI, "INIT wifi");

	//Inizializza stack TCPIP
	tcpip_adapter_init();

	//Crea un gruppo di eventi
	esp_event_loop_init(WIFI_event_handler, NULL);
	WIFI_event_group = xEventGroupCreate();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	//Inizializza WIFI
	esp_wifi_init(&cfg);
	esp_wifi_set_storage(WIFI_STORAGE_RAM);


	esp_wifi_set_mode(WIFI_MODE_STA);

	ESP_LOGD(TAG_WIFI, "Start WiFi");

	//Imposta la configurazione per il WIFI
	wifi_config_t sta_config = {
		.sta = {
			.bssid_set = false
		}
	};

	memset(sta_config.sta.ssid, 0x0, sizeof(sta_config.sta.ssid));
	memcpy(sta_config.sta.ssid, "Vodafone-34532257Vodafone-345322", strlen("Vodafone-34532257Vodafone-345322"));

	memset(sta_config.sta.password, 0x0, sizeof(sta_config.sta.password));
	memcpy(sta_config.sta.password, "12312312312323543456457567567sdfsadadasda2131231sadasdasdasdas21", strlen("12312312312323543456457567567sdfsadadasda2131231sadasdasdasdas21"));

	esp_wifi_set_config(WIFI_IF_STA, &sta_config);




	//Start WIFI
	esp_wifi_start();

	//Connetti alla rete
	esp_wifi_connect();






	WIFI_isConnected = false;

	WIFI_ping_check = 0;
	WIFI_InternetAvailable = false;


}

