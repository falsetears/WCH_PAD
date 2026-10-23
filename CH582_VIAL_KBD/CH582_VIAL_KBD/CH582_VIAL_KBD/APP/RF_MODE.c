/*
 * RF_MODE.c
 *
 *  Created on: 2024年11月6日
 *      Author: OWNER
 */
#include "CONFIG.h"
#include "RF_MODE.h"
#include "scan_key.h"
#include "ws2812.h"
#include "VIAL.h"
/*********************************************************************
 * GLOBAL TYPEDEFS
 */
uint8_t RF_taskID;
uint8_t TX_DATA[10] = {0x98,0x01,0, 0, 0x00, 0, 0, 0, 0, 0};
uint8_t TX_DATA1[3] = {0x98,0x02,0x00};
uint8_t TX_DATA3[1] = {0x96};
uint8_t TX_DATA4[20] = {0x98,0x03,0, 0, 0x00, 0, 0, 0, 0, 0};
uint16_t dongle_boot_flag = 0;

uint8_t  RF_Bounding_flag = 0;
uint8_t  keyboard_mac[6] = {0};
uint8_t  dongle_mac[6] = {0};
uint32_t keyboard_mac_32 = 0;
uint32_t dongle_mac_32 = 0;
uint32_t bounding_mac = 0;
volatile uint16_t rf_lost_count = 0;
uint8_t  rf_led_state[2] = {0};
uint8_t  TX_BIND[8] = {0x98,0xBD,0,0,0,0,0,0};

static uint8_t RF_ConfigAddress(uint32_t addr);
/*********************************************************************
 * @fn      RF_2G4StatusCallBack
 *
 * @brief   RF 状态回调，注意：不可在此函数中直接调用RF接收或者发送API，需要使用事件的方式调用
 *
 * @param   sta     - 状态类型
 * @param   crc     - crc校验结果
 * @param   rxBuf   - 数据buf指针
 *
 * @return  none
 */
void RF_2G4StatusCallBack(uint8_t sta, uint8_t crc, uint8_t *rxBuf)
{
    switch(sta)
    {
        case TX_MODE_TX_FINISH:
        {
            if (RF_Bounding_flag) {
                tmos_start_task(RF_taskID, SBP_RF_RF_RX_EVT, MS1_TO_SYSTEM_TIME(1));
            }
            break;
        }
        case TX_MODE_TX_FAIL:
        {
            if (RF_Bounding_flag) {
                tmos_start_task(RF_taskID, SBP_RF_RF_RX_EVT, MS1_TO_SYSTEM_TIME(1));
            }
            break;
        }
        case TX_MODE_RX_DATA:
        {
            if (crc == 0) {
                rf_lost_count = 0;
                if (rxBuf[2] == 0x98) {
                    if (rxBuf[3] == 0xBD) {          // 接收器绑定应答: 0x98 0xBD + dongle MAC
                        dongle_mac_32 = ((uint32_t)rxBuf[4] << 24) |
                                        ((uint32_t)rxBuf[5] << 16) |
                                        ((uint32_t)rxBuf[6] << 8)  |
                                        ((uint32_t)rxBuf[7]);
                        tmos_memcpy(dongle_mac, &rxBuf[4], 6);
                        if (!RF_Bounding_flag) {
                            bounding_mac = keyboard_mac_32 + dongle_mac_32;
                            tmos_set_event(RF_taskID, SBP_RF_BIND_EVT);
                        }
                    }
                    else if (rxBuf[3] == 0xEB) {     // 接收器心跳: 0x98 0xEB led0 led1
                        rf_led_state[0] = rxBuf[4];
                        rf_led_state[1] = rxBuf[5];
                    }
                }
            }
            if (RF_Bounding_flag) {
                tmos_start_task(RF_taskID, SBP_RF_RF_RX_EVT, MS1_TO_SYSTEM_TIME(1));
            }
            break;
        }
        case TX_MODE_RX_TIMEOUT:
        {
            if (RF_Bounding_flag) {
                tmos_start_task(RF_taskID, SBP_RF_RF_RX_EVT, MS1_TO_SYSTEM_TIME(1));
            }
            break;
        }
        case RX_MODE_RX_DATA:
        {
            if (crc == 0) {
                rf_lost_count = 0;
                if (rxBuf[2] == 0x98 && rxBuf[3] == 0xEB) {   // 心跳
                    rf_led_state[0] = rxBuf[4];
                    rf_led_state[1] = rxBuf[5];
                }
            }
            tmos_start_task(RF_taskID, SBP_RF_RF_RX_EVT, MS1_TO_SYSTEM_TIME(1));
            break;
        }
        case RX_MODE_TX_FINISH:
        {
            tmos_start_task(RF_taskID, SBP_RF_RF_RX_EVT, MS1_TO_SYSTEM_TIME(1));
            break;
        }
        case RX_MODE_TX_FAIL:
        {
            tmos_start_task(RF_taskID, SBP_RF_RF_RX_EVT, MS1_TO_SYSTEM_TIME(1));
            break;
        }
    }
}

/*********************************************************************
 * @fn      RF_ProcessEvent
 *
 * @brief   RF 事件处理
 *
 * @param   task_id - 任务ID
 * @param   events  - 事件标志
 *
 * @return  未完成事件
 */
uint16_t RF_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & SBP_RF_BIND_EVT)
    {
        if(!RF_Bounding_flag)
        {
            uint8_t state = RF_ConfigAddress(bounding_mac);
            PRINT("rf bind cfg: %x\n", state);
            if(state == 0)
            {
                RF_Bounding_flag = 1;
                rf_lost_count = 0;
                RF_Shut();
                RF_Rx(TX_DATA, 10, 0xFF, 0xFF);     // 绑定后作为从机，用当前键盘报告作为ACK
                PRINT("rf bound: %x\n", bounding_mac);
                tmos_start_task(RF_taskID, SBP_RF_kbd_data_EVT, MS1_TO_SYSTEM_TIME(10));
            }
            else
            {
                RF_ConfigAddress(0x82531829);
            }
        }
        return (events ^ SBP_RF_BIND_EVT);
    }
    if(events & SBP_RF_REBIND_EVT)
    {
        RF_Bounding_flag = 0;
        rf_lost_count = 0;
        RF_ConfigAddress(0x82531829);
        RF_Shut();
        RF_Tx(TX_BIND, 8, 0xFF, 0xFF);
        PRINT("rf rebind\n");
        tmos_start_task(RF_taskID, SBP_RF_kbd_data_EVT, MS1_TO_SYSTEM_TIME(8));
        return (events ^ SBP_RF_REBIND_EVT);
    }
    if(events & SBP_RF_RF_RX_EVT)
    {
        if(RF_Bounding_flag)
        {
            RF_Shut();
            RF_Rx(TX_DATA, 10, 0xFF, 0xFF);
        }
        return (events ^ SBP_RF_RF_RX_EVT);
    }
    if(events & SBP_RF_kbd_data_EVT)
    {
        if(!RF_Bounding_flag)
        {
            RF_Shut();
        }
        tmos_memset(scan_buf, 0, 6);
        scan_flag = get_key_fanz(scan_buf);

        tmos_memcpy(&TX_DATA[4], scan_buf, 6);

        if(!RF_Bounding_flag)
        {
            RF_Tx(TX_BIND, 8, 0xFF, 0xFF);
        }
        else
        {
            // 绑定后靠接收器心跳维持连接；1.6s 收不到心跳就重新绑定
            if(rf_lost_count < 60000)
            {
                rf_lost_count++;
            }
            if(rf_lost_count >= 200)
            {
                rf_lost_count = 0;
                tmos_set_event(RF_taskID, SBP_RF_REBIND_EVT);
            }
        }

        if (tmos_memcmp(last_buf, scan_buf, 6) == TRUE) {
            if (scan_flag == 0) {
                change_mode_USB = 0;
                change_mode_BLE = 0;
                dongle_boot_flag = 0;
            }
            else if (scan_flag == 1) {
                if (scan_buf[0]==key_data_buf[1][0]) {
                    //USB MODE
                    change_mode_USB++;
                }
                else if (scan_buf[0]==key_data_buf[1][1]) {
                    //ble MODE
                    change_mode_BLE++;
                }
                else if (scan_buf[0]==key_data_buf[2][1]) {
                    //dongle boot
                    dongle_boot_flag++;
                }
            }
        }
        else {
            change_mode_USB  = 0;
            change_mode_BLE  = 0;
            dongle_boot_flag = 0;
        }
        tmos_memcpy(last_buf, scan_buf, 6);

        if (change_mode_USB == 300) {
            uint8_t key[1] = {0x0B};
            FLASH_DATA_VIAL_WITE_mode(key);
            DelayMs(1);
            SYS_ResetExecute();
        }
        if (change_mode_BLE == 300) {
            uint8_t key[1] = {0xBE};
            FLASH_DATA_VIAL_WITE_mode(key);
            DelayMs(1);
            SYS_ResetExecute();
        }
        if (dongle_boot_flag == 1250) {
            tmos_start_task(RF_taskID, SBP_dongle_boot_EVT, 0);
        }
        tmos_start_task(RF_taskID, SBP_RF_kbd_data_EVT, MS1_TO_SYSTEM_TIME(8));
        return (events ^ SBP_RF_kbd_data_EVT);
    }
    if(events & SBP_dongle_boot_EVT)
    {
       RF_Shut();
       RF_Tx(TX_DATA3, 1, 0xFF, 0xFF);
       if(RF_Bounding_flag)
       {
           tmos_start_task(RF_taskID, SBP_RF_RF_RX_EVT, MS1_TO_SYSTEM_TIME(2));
       }
       return (events ^ SBP_dongle_boot_EVT);
    }
    if(events & WS2812_EVENT)
    {
       process_RGB_to_pwm(flowing_buf, 17, Pwmout_buf);
       Ws2812_move_control(flowing_buf,RGB_Left_flowing_water,17);//模式选择
       PWM_DATA_DMA_send(Pwmout_buf,sizeof(Pwmout_buf));
       tmos_start_task(RF_taskID, WS2812_EVENT, MS1_TO_SYSTEM_TIME(150));
       return (events ^ WS2812_EVENT);
    }

    return 0;
}

/*********************************************************************
 * @fn      RF_ConfigAddress
 *
 * @brief   配置 RF 访问地址、频点和回调
 *
 * @return  0 - 成功
 */
static uint8_t RF_ConfigAddress(uint32_t addr)
{
    rfConfig_t cfg;
    uint8_t state;

    tmos_memset(&cfg, 0, sizeof(rfConfig_t));
    cfg.accessAddress = addr;
    cfg.CRCInit = 0x555555;
    cfg.Channel = 8;
    cfg.Frequency = 2405000;
    cfg.LLEMode = LLE_MODE_AUTO | LLE_MODE_EX_CHANNEL;
    cfg.rfStatusCB = RF_2G4StatusCallBack;
    cfg.RxMaxlen = 251;

    RF_Shut();
    state = RF_Config(&cfg);
    return state;
}

/*********************************************************************
 * @fn      RF_Init
 *
 * @brief   RF 初始化，读取键盘 MAC 并进入绑定流程
 *
 * @return  none
 */
void RF_Init(void)
{
    __attribute__((aligned(4))) uint8_t mac[8] = {0};

    RF_taskID = TMOS_ProcessEventRegister(RF_ProcessEvent);

    GetMACAddress(mac);
    tmos_memcpy(keyboard_mac, mac, 6);
    keyboard_mac_32 = ((uint32_t)mac[0] << 24) |
                      ((uint32_t)mac[1] << 16) |
                      ((uint32_t)mac[2] << 8)  |
                      ((uint32_t)mac[3]);
    tmos_memcpy(&TX_BIND[2], mac, 6);

    RF_Bounding_flag = 0;
    rf_lost_count = 0;
    RF_ConfigAddress(0x82531829);

    PRINT("rf bind mac: %x\n", keyboard_mac_32);
    tmos_start_task(RF_taskID, SBP_RF_kbd_data_EVT, MS1_TO_SYSTEM_TIME(8));
}
/******************************** endfile @ main ******************************/
