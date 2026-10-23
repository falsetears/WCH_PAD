/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH_yz
 * Version            : V1.0
 * Date               : 2024/04/10
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: GPL-3.0
 *******************************************************************************/

/******************************************************************************/
/* ͷ�ļ����� */
#include "CONFIG.h"
#include "RF_Dongle.h"
#include "USB_BLE.h"
/*********************************************************************
 * GLOBAL TYPEDEFS
 */
tmosTaskID RFtaskID;
uint8_t RF_Bounding_flag = 0;
uint8_t time_out_flag = 0;
extern Dongle_data_STRUCT Dongle_data;
extern uint8_t vial_out_flag;
volatile uint8_t tx_end_flag=0;
volatile uint8_t rx_end_flag=0;
uint32_t bounding_mac = 0;
uint32_t dongle_mac_32 = 0;
uint32_t device_mac_32 = 0;
__attribute__((aligned(4))) uint8_t device_mac[6] = {0};
__attribute__((aligned(4))) uint8_t dongle_mac[8] = {0x98,0xBD,0X00,0X00,0X00,0X00,0X00,0X00};
__attribute__((aligned(4))) uint8_t USB_VIAL_DATA[36] = {0x98,0xE2,0X00,0X00,0X00,0X00,0X00,0X00};

static void Dongle_BindFlash_Write(uint32_t mac)
{
    __attribute__((aligned(4))) uint32_t buf[1] = {mac};
    EEPROM_WRITE(RF_BIND_FLASH_ADDR, buf, 4);
}

static uint32_t Dongle_BindFlash_Read(void)
{
    __attribute__((aligned(4))) uint32_t buf[1] = {0};
    EEPROM_READ(RF_BIND_FLASH_ADDR, buf, 4);
    if (buf[0] == 0xFFFFFFFF || buf[0] == 0) return 0;
    return buf[0];
}

static void Dongle_BindFlash_Erase(void)
{
    EEPROM_ERASE(RF_BIND_FLASH_ADDR, 256);
}

void RF_BootInit(void)
{
    uint32_t saved = Dongle_BindFlash_Read();
    if (saved != 0) {
        bounding_mac = saved;
        RF_Bounding_flag = 1;
        PRINT("dongle restored binding: %x\n", saved);
        RF_Init(saved);
    } else {
        RF_Init(0x82531829);
    }
}
/*********************************************************************
 * @fn      RF_Wait_Tx_End
 *
 * @brief   �ֶ�ģʽ�ȴ�������ɣ��Զ�ģʽ�ȴ�����-������ɣ�������RAM�еȴ����ȴ�ʱ����ִ���û����룬����Ҫע��ִ�еĴ������������RAM�У�����Ӱ�췢��
 *
 * @return  none
 */
__HIGH_CODE
__attribute__((noinline))
void RF_Wait_Tx_End()
{
    uint32_t i=0;
    while(!tx_end_flag)
    {
        i++;
        __nop();
        __nop();
        // Լ5ms��ʱ
        if(i>(FREQ_SYS/1000))
        {
            tx_end_flag = TRUE;
        }
    }
}

/*********************************************************************
 * @fn      RF_Wait_Rx_End
 *
 * @brief   �Զ�ģʽ�ȴ�Ӧ������ɣ�������RAM�еȴ����ȴ�ʱ����ִ���û����룬����Ҫע��ִ�еĴ������������RAM�У�����Ӱ�췢��
 *
 * @return  none
 */
__HIGH_CODE
__attribute__((noinline))
void RF_Wait_Rx_End()
{
    uint32_t i=0;
    while(!rx_end_flag)
    {
        i++;
        __nop();
        __nop();
        // Լ5ms��ʱ
        if(i>(FREQ_SYS/1000))
        {
            rx_end_flag = TRUE;
        }
    }
}
/*********************************************************************
 * @fn      RF_2G4StatusCallBack
 *
 * @brief   RF ״̬�ص���ע�⣺�����ڴ˺�����ֱ�ӵ���RF���ջ��߷���API����Ҫʹ���¼��ķ�ʽ����
 *
 * @param   sta     - ״̬����
 * @param   crc     - crcУ����
 * @param   rxBuf   - ����bufָ��
 *
 * @return  none
 */
void RF_2G4StatusCallBack(uint8_t sta, uint8_t crc, uint8_t *rxBuf)
{
    switch(sta)
    {

        case TX_MODE_TX_FINISH:
        {
            break;
        }
        case TX_MODE_TX_FAIL:
        {
            break;
        }
        case TX_MODE_RX_DATA:
        {
            time_out_flag = 0;
//           PRINT("rx\r\n");
            if (crc == 0) {
                if(rxBuf[2]==0x98) {

                    if (rxBuf[3]==0x01) {     //keyboard
                       tmos_memcpy(Dongle_data.RXBUFF_key,&rxBuf[4], 8);
                       tmos_set_event(usbTaskID, USB_KEYBOARD_EVENT);
                    }
                    else if(rxBuf[3]==0x02){  //vol
                       tmos_memcpy(Dongle_data.RXBUFF_vol,&rxBuf[4], 3);
                       tmos_set_event(usbTaskID, USB_VOL_EVENT);
                    }
                    else if(rxBuf[3]==0xE2){
                        PRINT("rxBuf:%d",rxBuf[1]);
                        for (int ar = 0; ar < 32; ++ar) {
                           PRINT("%x ",rxBuf[ar+4]);
                        }
                        PRINT("\r\n");
                        vial_out_flag = 0;
                        tmos_memcpy(Dongle_data.RXBUFF_vol,&rxBuf[4], 32);
                        tmos_set_event(usbTaskID, USB_VIAL_EP2_EVENT);
                    }
                    else if(rxBuf[3]==0xBE){
                        tmos_set_event(RFtaskID, SBP_unbind_EVT);
                    }
               }
               else if(rxBuf[2]==0x96)       //jump boot and ISP
               {
                   PRINT("jump boot\r\n");
                   APPJumpBoot();
               }
            }
            break;
        }
        case TX_MODE_RX_TIMEOUT: // Timeout is about 200us
        {
            time_out_flag++;
            if (time_out_flag>=200) {
                RF_Bounding_flag = 0;
                time_out_flag=0;
                tmos_memset(Dongle_data.RXBUFF_key, 0, 8); //�Ͽ�����һ��
                tmos_set_event(usbTaskID, USB_KEYBOARD_EVENT);
                tmos_stop_task(RFtaskID, SBP_heartbeat_EVT);
                tmos_start_task(RFtaskID, SBP_nobounding_EVT,0);
            }
            break;
        }
        case RX_MODE_RX_DATA:
        {
            if (crc == 0) {
                   RF_Wait_Rx_End();
                   if(rxBuf[2]==0x98) {
                       if(rxBuf[3]==0xBD){  //Bounding
                            tmos_memcpy(device_mac,&rxBuf[4],6);
                            device_mac_32 = (device_mac[0]<<24)+(device_mac[1]<<16)+(device_mac[2]<<8)+device_mac[3];
                            PRINT("device_mac_32: %x\n",device_mac_32);
                            tmos_set_event(RFtaskID, SBP_bounding_EVT);
                       }
                   }
               }
            tmos_set_event(RFtaskID, SBP_RF_RF_RX_EVT);
            break;
        }
        case RX_MODE_TX_FINISH:
        {
            tmos_set_event(RFtaskID, SBP_RF_RF_RX_EVT);
            break;
        }
        case RX_MODE_TX_FAIL:
        {
            tmos_set_event(RFtaskID, SBP_RF_RF_RX_EVT);
            break;
        }
    }
}

/*********************************************************************
 * @fn      RF_ProcessEvent
 *
 * @brief   RF �¼�����
 *
 * @param   task_id - ����ID
 * @param   events  - �¼���־
 *
 * @return  δ����¼�
 */
uint16_t RF_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(task_id)) != NULL)
        {
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    if(events & SBP_bounding_EVT)
    {
        RF_Shut();
        bounding_mac = device_mac_32+dongle_mac_32;
        RF_Bounding_flag = 1;
        Dongle_BindFlash_Write(bounding_mac);
        RF_Init(bounding_mac);
//        tmos_start_task( usbTaskID, START_USB_EVENT, 0 );//开usb中断
        return events ^ SBP_bounding_EVT;
    }
    if(events & SBP_nobounding_EVT)
    {
        RF_Shut();
        RF_Bounding_flag = 0;
        RF_Init(0x82531829);
//        tmos_start_task( usbTaskID, STOP_USB_EVENT, 0 );//关闭usb中断
        return events ^ SBP_nobounding_EVT;
    }
    if(events & SBP_unbind_EVT)
    {
        RF_Shut();
        RF_Bounding_flag = 0;
        Dongle_BindFlash_Erase();
        tmos_stop_task(RFtaskID, SBP_heartbeat_EVT);
        RF_Init(0x82531829);
        PRINT("dongle unbind\n");
        return events ^ SBP_unbind_EVT;
    }
    if(events & SBP_RF_RF_RX_EVT)
    {
        RF_Shut();
        rx_end_flag = FALSE;
        RF_Rx(dongle_mac, 8, 0xFF, 0xFF);

        return events ^ SBP_RF_RF_RX_EVT;
    }
    if(events & SBP_heartbeat_EVT)
    {
        RF_Shut();
        if (vial_out_flag == 1) {
            vial_out_flag = 0;
            uint8_t heartbeat0[34];
            heartbeat0[0] = 0x98;
            heartbeat0[1] = 0xe2;
            memcpy(&heartbeat0[2],Dongle_data.RX_VIAL_DATA,32);
            RF_Tx(heartbeat0, 34, 0xFF, 0xFF);
        }
        else {
            uint8_t heartbeat[4];
            heartbeat[0] = 0x98;
            heartbeat[1] = 0xeb;
            heartbeat[2] = Dongle_data.led[0];
            heartbeat[3] = Dongle_data.led[1];
            RF_Tx(heartbeat, 4, 0xFF, 0xFF);
        }
        tmos_start_task(RFtaskID, SBP_heartbeat_EVT, MS1_TO_SYSTEM_TIME(10));
        return events ^ SBP_heartbeat_EVT;
    }
    if(events & SBP_USB_EP3RX_EVT)
    {
//        tmos_stop_task(RFtaskID, SBP_heartbeat_EVT);
        RF_Shut();
        USB_VIAL_DATA[0] = 0X98;
        USB_VIAL_DATA[1] = 0XE2;
        memcpy(&USB_VIAL_DATA[2],Dongle_data.RX_VIAL_DATA,32);
//        PRINT("endp 0 data:");
//        for (int ar = 0; ar < 34; ++ar) {
//            PRINT("%x ",USB_VIAL_DATA[ar]);
//        }
//        PRINT("\r\n");
        RF_Tx(USB_VIAL_DATA, 34, 0xFF, 0xFF);
        tmos_start_task(RFtaskID, SBP_USB_EP3RX_EVT, MS1_TO_SYSTEM_TIME(10));
        return events ^ SBP_USB_EP3RX_EVT;
    }
    return 0;
}

/*********************************************************************
 * @fn      RF_Init
 *
 * @brief   RF ��ʼ��
 *
 * @return  none
 */
void RF_Init(uint32_t RF_ADD)
{
    rfConfig_t rfConfig;
    tmos_memset(&rfConfig, 0, sizeof(rfConfig_t));
    RFtaskID = TMOS_ProcessEventRegister(RF_ProcessEvent);

    rfConfig.accessAddress = RF_ADD;
    rfConfig.CRCInit = 0x555555;
    rfConfig.Channel = 8;
    rfConfig.Frequency = 2405000;
    rfConfig.LLEMode = LLE_MODE_AUTO | LLE_MODE_EX_CHANNEL;
    rfConfig.rfStatusCB = RF_2G4StatusCallBack;
    rfConfig.RxMaxlen = 251;
    RF_Config(&rfConfig);

    if (RF_Bounding_flag == 0) {
        __attribute__((aligned(4))) uint8_t dongle_mac_temp[8];
        GetMACAddress(dongle_mac_temp);
        memcpy(&dongle_mac[2],dongle_mac_temp,6);
        dongle_mac_32 =(dongle_mac_temp[0]<<24)+(dongle_mac_temp[1]<<16)+(dongle_mac_temp[2]<<8)+dongle_mac_temp[3];
        PRINT("rf 2.4g init MAC32: %x\n",dongle_mac_32);
        { // RX mode
            rx_end_flag = FALSE;
            RF_Rx(dongle_mac, 8, 0xFF, 0xFF);
        }
    }
    else {
        tmos_start_task(RFtaskID, SBP_heartbeat_EVT, MS1_TO_SYSTEM_TIME(1));
    }

}
/******************************** endfile @ main ******************************/
