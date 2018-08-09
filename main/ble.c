/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include "glob_inc.h"



static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x100,
    .scan_window            = 0x080
};

#define BLE_TIMEOUT_INCATIVITY		300	//30 secondi di timeout di inattività

#define BLE_PROFILE_NUM				1
#define BLE_PROFILE_APP_IDX			0
#define BLE_APP_ID					0x55
#define BLE_DEFAULT_NAME			"Test1"
#define SVC_INST_ID                 0

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 100
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

#define BLE_ADV_SCAN_POWER_POS		9
#define BLE_ADV_SCAN_NAME_POS		12

static uint8_t adv_config_done       = 0;

uint16_t BLE_handle_table[HRS_IDX_NB];

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;


static  uint8_t raw_adv_data[31] = {
	0x02, 0x01, 0x06, //3
	0x03, 0x03, 0xAA, 0xFE, //4
	0x17, 0x16, 0xAA, 0xFE, //4
	0x00, // 1	//Frame type
	0x00, // 1 //Tx power at 0m
    0xe5, 0xf8, 0xf4, 0x9b, 0xf5, 0x61, 0xfc, 0x61, 0xf6, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //16 //NAMESPACE Id + INSTANCE Id
    0x00, 0x00 //2
};

static uint8_t raw_scan_rsp_data[] = {
        //Appearance (384 generic remote control)
        0x03, 0x19, 0x80, 0x01,
		/* flags */
        0x02, 0x01, 0x06,
        /* tx power */
        0x02, 0x0a, 0xdb,
        /* Device name */
        0x06, 0x09, 'T', 'e', 's', 't', '1'
};




static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst BLE_profile_tab[BLE_PROFILE_NUM] = {
    [BLE_PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Service */
static const uint16_t GATTS_SERVICE_UUID			= 0x00FF;
static const uint16_t GATTS_CHAR_UUID_APPID			= 0xFF01; //scrivere in big-endian rispetto all UUID del servizio.
static const uint16_t GATTS_CHAR_UUID_COMMAND		= 0xFF02;
static const uint16_t GATTS_CHAR_UUID_REPLY			= 0xFF03;

static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t char_prop_read                = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;


static uint8_t BLE_APPID_value[21] = {0x00};
static uint8_t BLE_Command_value[301] = {0x00};
static uint8_t BLE_Reply_value[101] = {0x22};



//Full Database Description - Used to insert attributes into the database
static const esp_gatts_attr_db_t gatt_db[HRS_IDX_NB] =
{

    // Service Declaration
    [IDX_SVC]	=
    {
    		{ESP_GATT_AUTO_RSP},
    		{ESP_UUID_LEN_16,
    		(uint8_t *)&primary_service_uuid,
    		ESP_GATT_PERM_READ,
    		sizeof(GATTS_SERVICE_UUID),
    		sizeof(GATTS_SERVICE_UUID),
    		(uint8_t *)&GATTS_SERVICE_UUID
    		}
    },

    /* Characteristic Declaration (APPID) */
	[IDX_CHAR_APPID]	=
	{
			{ESP_GATT_AUTO_RSP},
			{ESP_UUID_LEN_16,
			(uint8_t *)&character_declaration_uuid,
			ESP_GATT_PERM_READ,
			CHAR_DECLARATION_SIZE,
			CHAR_DECLARATION_SIZE,
			(uint8_t *)&char_prop_write
			}
	},

	/* Characteristic Value (APPID) */
	[IDX_CHAR_APPID_VAL]  =
    {
    		{ESP_GATT_AUTO_RSP},
    		{ESP_UUID_LEN_16,
    		(uint8_t *)&GATTS_CHAR_UUID_APPID,
    		ESP_GATT_PERM_WRITE,
    		21,
    		sizeof(BLE_APPID_value),
    		(uint8_t *)BLE_APPID_value
    		}
    },


    /* Characteristic Declaration (COMMAND) */
    [IDX_CHAR_CMD]      =
    {
    		{ESP_GATT_AUTO_RSP},
    		{ESP_UUID_LEN_16,
    		(uint8_t *)&character_declaration_uuid,
    		ESP_GATT_PERM_READ,
    		CHAR_DECLARATION_SIZE,
    		CHAR_DECLARATION_SIZE,
    		(uint8_t *)&char_prop_write
    		}
    },

    /* Characteristic Value (COMMAND) */
    [IDX_CHAR_CMD_VAL]  =
    {
    		{ESP_GATT_AUTO_RSP},
    		{ESP_UUID_LEN_16,
    		(uint8_t *)&GATTS_CHAR_UUID_COMMAND,
    		ESP_GATT_PERM_WRITE,
    		301,
    		sizeof(BLE_Command_value),
    		(uint8_t *)BLE_Command_value
    		}
    },

    /* Characteristic Declaration (REPLY) */
    [IDX_CHAR_REPLY]      =
    {
    		{ESP_GATT_AUTO_RSP},
    		{ESP_UUID_LEN_16,
    		(uint8_t *)&character_declaration_uuid,
    		ESP_GATT_PERM_READ,
    		CHAR_DECLARATION_SIZE,
    		CHAR_DECLARATION_SIZE,
    		(uint8_t *)&char_prop_read
    		}
    },

    /* Characteristic Value (REPLY) */
    [IDX_CHAR_REPLY_VAL]  =
    {
    		{ESP_GATT_RSP_BY_APP},
    		{ESP_UUID_LEN_16,
    		(uint8_t *)&GATTS_CHAR_UUID_REPLY,
    		ESP_GATT_PERM_READ,
    		101,
    		sizeof(BLE_Reply_value),
    		(uint8_t *)BLE_Reply_value
    		}
    },

};

static void BLE_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {

        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0 ){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0 ){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG_GATTS, "advertising start failed");
            }else{
                ESP_LOGD(TAG_GATTS, "advertising start successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG_GATTS, "Advertising stop failed");
            }
            else {
                ESP_LOGD(TAG_GATTS, "Stop adv successfully\n");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGD(TAG_GATTS, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;

        //Observer mode
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:

        	if(0)
        	{
        		uint32_t duration = 0;
        		esp_ble_gap_start_scanning(duration);
        	}
		   break;

	   case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
		   if(param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
			   ESP_LOGE(TAG_BLE,"Scan start failed");
		   }
		   else {
			   ESP_LOGD(TAG_BLE,"Start scanning...");
		   }
		   break;
	   }
	   case ESP_GAP_BLE_SCAN_RESULT_EVT: {
		   esp_ble_gap_cb_param_t* scan_result = (esp_ble_gap_cb_param_t*)param;
		   switch(scan_result->scan_rst.search_evt)
		   {
			   case ESP_GAP_SEARCH_INQ_RES_EVT: {

				   ESP_LOGD(TAG_BLE, "ESP_GAP_SEARCH_INQ_RES_EVT");

				   break;
			   }
			   default:
				   break;
		   }
		   break;
	   }
	   case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:{
		   if(param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
		   {
			   ESP_LOGE(TAG_BLE,"Scan stop failed");
		   }
		   else
		   {
			   ESP_LOGD(TAG_BLE,"Stop scan successfully");
		   }
		   break;
	   }

        default:
            break;
    }
}

void example_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGD(TAG_GATTS, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;
    if (prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(TAG_GATTS, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    } else {
        if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(TAG_GATTS, "Send response error");
            }
            free(gatt_rsp);
        }else{
            ESP_LOGE(TAG_GATTS, "%s, malloc failed", __func__);
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
        esp_log_buffer_hex(TAG_GATTS, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGD(TAG_GATTS,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

//Eventi del PROFILO APP
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
    	//Evento di registrazione
        case ESP_GATTS_REG_EVT:
        {
        	ESP_LOGD(TAG_GATTS, "ESP_GATTS_REG_EVT");

        	esp_err_t set_dev_name_ret;

       		set_dev_name_ret = esp_ble_gap_set_device_name(BLE_DEFAULT_NAME);

			if (set_dev_name_ret)
			{
				ESP_LOGE(TAG_GATTS, "set device name failed, error code = %x", set_dev_name_ret);
			}



			for (uint8_t j = 0; j < 31; j++)
			{
				ESP_LOGD(TAG_GATTS, " post UID[%d]:%02X", j, *(raw_adv_data + j));
			}

			//Setup raw data
            esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
            if (raw_adv_ret){
                ESP_LOGE(TAG_GATTS, "config raw adv data failed, error code = %x ", raw_adv_ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;





            esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
            if (raw_scan_ret){
                ESP_LOGE(TAG_GATTS, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;

            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(TAG_GATTS, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGD(TAG_GATTS, "ESP_GATTS_READ_EVT;Han:%d;need_rsp:%d", param->read.handle, param->read.need_rsp);
            ESP_LOGD(TAG_GATTS, "ble_reply:%s", BLE_Reply_value);

            ESP_LOGD(TAG_GATTS, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = strlen((char*) BLE_Reply_value);
            memset(rsp.attr_value.value, 0x0, sizeof(rsp.attr_value.value));
            memcpy(rsp.attr_value.value, BLE_Reply_value, strlen((char*) BLE_Reply_value));

            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);


       	    break;
        case ESP_GATTS_WRITE_EVT:
//        	res = find_char_and_desr_index(param->write.handle);
//        	if(param->write.is_prep == false){

        	ESP_LOGD(TAG_GATTS, "GATT_WRITE_EVT %d", param->write.is_prep);

            if (!param->write.is_prep)
            {
                ESP_LOGD(TAG_GATTS, "GATT_WRITE_EVT,handle=%d,len=%d,value:%s", param->write.handle, param->write.len, param->write.value);

                if (param->write.handle == BLE_handle_table[IDX_CHAR_APPID_VAL])
                {
					memset(BLE_APPID_value, 0x0, sizeof(BLE_APPID_value));
					memcpy(BLE_APPID_value, param->write.value, param->write.len);

					ESP_LOGD(TAG_GATTS, "BLE_APPID_value:%s", BLE_APPID_value);


                }
                else if (param->write.handle == BLE_handle_table[IDX_CHAR_CMD_VAL])
                {
            		memset(BLE_Command_value, 0x0, sizeof(BLE_Command_value));
                	memcpy(BLE_Command_value, (char*)param->write.value, param->write.len);
           			ESP_LOGD(TAG_GATTS, "BLE_Command_value:%s", BLE_Command_value);
                }

                /* send response when param->write.need_rsp is true*/
                if (param->write.need_rsp)
                {

                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }else{
                /* handle prepare write */
                example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            ESP_LOGD(TAG_GATTS, "ESP_GATTS_EXEC_WRITE_EVT");
            example_exec_write_event_env(&prepare_write_env, param);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGD(TAG_GATTS, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGD(TAG_GATTS, "ESP_GATTS_CONF_EVT, status = %d", param->conf.status);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGD(TAG_GATTS, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGD(TAG_GATTS, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);

            esp_log_buffer_hex(TAG_GATTS, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGD(TAG_GATTS, "ESP_GATTS_DISCONNECT_EVT, reason = %d", param->disconnect.reason);
            	esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(TAG_GATTS, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != HRS_IDX_NB){
                ESP_LOGE(TAG_GATTS, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, HRS_IDX_NB);
            }
            else {
                ESP_LOGD(TAG_GATTS, "create attribute table successfully, the number handle = %d", param->add_attr_tab.num_handle);
                for (uint8_t i = 0; i < param->add_attr_tab.num_handle; i++)
                {
                	ESP_LOGD(TAG_GATTS, "attr hndl[%d]=%d", i, param->add_attr_tab.handles[i]);
                }
                memcpy(BLE_handle_table, param->add_attr_tab.handles, sizeof(BLE_handle_table));
                esp_ble_gatts_start_service(BLE_handle_table[IDX_SVC]);
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        	ESP_LOGD(TAG_BLE, "ESP_GATTS_STOP_EVT");
        	break;
        case ESP_GATTS_OPEN_EVT:
        	ESP_LOGD(TAG_BLE, "ESP_GATTS_OPEN_EVT");
        	break;
        case ESP_GATTS_CANCEL_OPEN_EVT:
        	ESP_LOGD(TAG_BLE, "ESP_GATTS_CANCEL_OPEN_EVT");
			break;
        case ESP_GATTS_CLOSE_EVT:
        	ESP_LOGD(TAG_BLE, "ESP_GATTS_CLOSE_EVT");
        	break;
        case ESP_GATTS_LISTEN_EVT:
        	ESP_LOGD(TAG_BLE, "ESP_GATTS_LISTEN_EVT");
        	break;
        case ESP_GATTS_CONGEST_EVT:
        	ESP_LOGD(TAG_BLE, "ESP_GATTS_CONGEST_EVT");
        	break;
        case ESP_GATTS_UNREG_EVT:
        	ESP_LOGD(TAG_BLE, "ESP_GATTS_UNREG_EVT");
        	break;
        case ESP_GATTS_DELETE_EVT:
        	ESP_LOGD(TAG_BLE, "ESP_GATTS_DELETE_EVT");
        	break;
        default:
            break;
    }
}


static void BLE_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
        	BLE_profile_tab[BLE_PROFILE_APP_IDX].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGE(TAG_GATTS, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    do {
        int idx;
        for (idx = 0; idx < BLE_PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == BLE_profile_tab[idx].gatts_if)
            {
                if (BLE_profile_tab[idx].gatts_cb)
                {
                	BLE_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}




void Init_BLE_Service(void)
{
	//Bluetooth deve essere avviato
	ESP_LOGD(TAG_BLE, "start BLE");

	esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	esp_bt_controller_init(&bt_cfg);
	esp_bt_controller_enable(ESP_BT_MODE_BLE);

	esp_bluedroid_init();
	esp_bluedroid_enable();




	ESP_LOGD(TAG_BLE, "advertizer mode");

   esp_err_t ret = esp_ble_gatts_register_callback(BLE_gatts_event_handler);
	if (ret){
		ESP_LOGE(TAG_GATTS, "gatts register error, error code = %x", ret);
		return;
	}

	ret = esp_ble_gap_register_callback(BLE_gap_event_handler);
	if (ret){
		ESP_LOGE(TAG_GATTS, "gap register error, error code = %x", ret);
		return;
	}

	ret = esp_ble_gatts_app_register(BLE_APP_ID);
	if (ret){
		ESP_LOGE(TAG_GATTS, "gatts app register error, error code = %x", ret);
		return;
	}

	esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
	if (local_mtu_ret){
		ESP_LOGE(TAG_GATTS, "set local  MTU failed, error code = %x", local_mtu_ret);
	}


	ESP_LOGD(TAG_BLE, "observer mode");

	// set scan parameters
	esp_ble_gap_set_scan_params(&ble_scan_params);


	esp_err_t err = esp_ble_gap_start_advertising(&adv_params);
	ESP_LOGD(TAG_BLE, "start adv:%d", err);

}

