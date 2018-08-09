/***************************************************************************/
/*  File       : ble.h		                                               */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*  contents   : bluetooth low energy include file						   */
/*                                                                         */
/***************************************************************************/


/*------------------------------------------------------------------------

                    Common definitions

---------------------------------------------------------------------------*/

/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


typedef struct {
    uint8_t flags[3];
    uint8_t type[4];
    uint8_t service[5];
    int8_t measured_power;
    uint16_t uid[16];
    uint8_t tail[2];
}__attribute__((packed)) BLE_eddystone_t;


/* Attributes State Machine */
enum
{
    IDX_SVC,

    IDX_CHAR_APPID,
    IDX_CHAR_APPID_VAL,

    IDX_CHAR_CMD,
    IDX_CHAR_CMD_VAL,

    IDX_CHAR_REPLY,
    IDX_CHAR_REPLY_VAL,

    HRS_IDX_NB
};



#ifndef BLE

/*------------------------------------------------------------------------

                    Public variables   

---------------------------------------------------------------------------*/




/*------------------------------------------------------------------------

                    Public functions   

---------------------------------------------------------------------------*/
#endif



void Init_BLE_Service(void);


