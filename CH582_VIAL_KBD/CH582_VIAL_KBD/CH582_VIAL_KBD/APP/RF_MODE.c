/*
 * RF_MODE.c
 *
 *  Created on: 2024��11��6��
 *      Author: OWNER
 */
#include "CONFIG.h"
#include "RF_MODE.h"
#include "scan_key.h"
#include "ws2812.h"
#include "VIAL.h"

/* 2.4G status LED layout */
#define RF_LED_COUNT            17
#define RF_LED_LAST             (RF_LED_COUNT - 1)
#define RF_LED_REFRESH_MS       125
#define RF_LED_BLINK_HALF_MS    250
#define RF_LINK_LOST_LIMIT      100

#define RF_LED_YELLOW           0x080800
#define RF_LED_RED              0x080000
#define RF_LED_BLUE             0x000008
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
uint8_t  rf_pairing_mode = 0;
uint8_t  rf_led_blink_state = 0;
uint16_t change_mode_pair = 0;

static uint8_t RF_ConfigAddress(uint32_t addr);
static void RF_BindFlash_Write(uint32_t mac);
static uint32_t RF_BindFlash_Read(void);
static void RF_BindFlash_Erase(void);
/*********************************************************************
 * @fn      RF_BindFlash_Write
 *
 * @brief   Write 2.4G binding MAC to Data-Flash
 */
static void RF_BindFlash_Write(uint32_t mac)
{
    __attribute__((aligned(4))) uint32_t buf[1] = {mac};
    EEPROM_WRITE(RF_BIND_FLASH_ADDR, buf, 4);
}

/*********************************************************************
 * @fn      RF_BindFlash_Read
 *
 * @brief   Read 2.4G binding MAC from Data-Flash, returns 0 if not bound
 */
static uint32_t RF_BindFlash_Read(void)
{
    __attribute__((aligned(4))) uint32_t buf[1] = {0};
    EEPROM_READ(RF_BIND_FLASH_ADDR, buf, 4);
    if (buf[0] == 0xFFFFFFFF || buf[0] == 0) return 0;
    return buf[0];
}

/*********************************************************************
 * @fn      RF_BindFlash_Erase
 *
 * @brief   Erase 2.4G binding MAC from Data-Flash
 */
static void RF_BindFlash_Erase(void)
{
    EEPROM_ERASE(RF_BIND_FLASH_ADDR, 256);
}

/*********************************************************************
 * @fn      RF_2G4StatusCallBack
 *
 * @brief   RF ״̬صע⣺ڴֱӵRFջ߷APIҪʹ¼ķʽ
 *
 * @param   sta     - ״̬
 * @param   crc     - crcУ
 * @param   rxBuf   - ָ
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
                    if (rxBuf[3] == 0xBD) {          // ��������Ӧ��: 0x98 0xBD + dongle MAC
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
                    else if (rxBuf[3] == 0xEB) {     // ����������: 0x98 0xEB led0 led1
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
                if (rxBuf[2] == 0x98 && rxBuf[3] == 0xEB) {   // ����
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
 * @brief   RF �¼�����
 *
 * @param   task_id - ����ID
 * @param   events  - �¼���־
 *
 * @return  δ����¼�
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
                rf_pairing_mode = 0;
                RF_BindFlash_Write(bounding_mac);
                RF_Shut();
                RF_Rx(TX_DATA, 10, 0xFF, 0xFF);
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
        rf_lost_count = 100;
        if (bounding_mac != 0) {
            RF_Bounding_flag = 1;
            RF_ConfigAddress(bounding_mac);
            RF_Shut();
            RF_Rx(TX_DATA, 10, 0xFF, 0xFF);
            PRINT("rf wait original dongle: %x\n", bounding_mac);
        } else {
            RF_Bounding_flag = 0;
            RF_ConfigAddress(0x82531829);
            RF_Shut();
            RF_Tx(TX_BIND, 8, 0xFF, 0xFF);
            PRINT("rf rebind\n");
        }
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
            // �󶨺󿿽���������ά�����ӣ�1.6s �ղ������������°�
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
                change_mode_pair = 0;
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
                else if (scan_buf[0]==key_data_buf[4][3]) {
                    //Enter long press - pairing
                    change_mode_pair++;
                }
            }
        }
        else {
            change_mode_USB  = 0;
            change_mode_BLE  = 0;
            dongle_boot_flag = 0;
            change_mode_pair = 0;
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
        if (change_mode_pair == 300) {
            rf_pairing_mode = 1;
            RF_BindFlash_Erase();
            RF_Bounding_flag = 0;
            rf_lost_count = 0;
            bounding_mac = 0;
            RF_ConfigAddress(0x82531829);
            RF_Shut();
            RF_Tx(TX_BIND, 8, 0xFF, 0xFF);
            PRINT("rf pairing mode\n");
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
        static __attribute__((aligned(4))) uint32_t led_buf[RF_LED_COUNT];
        uint32_t base_color = RF_LED_YELLOW;
        uint32_t status_color = 0;
        uint8_t  led_on;

        /* 2 Hz blink: 250 ms on, 250 ms off, phase locked to the system clock */
        led_on = (uint8_t)(((TMOS_GetSystemClock() / MS1_TO_SYSTEM_TIME(RF_LED_BLINK_HALF_MS)) & 1) == 0);
        rf_led_blink_state = led_on;

        if (rf_pairing_mode) {
            /* Pairing: LEDs 1-16 yellow, last LED blue 2 Hz */
            status_color = led_on ? RF_LED_BLUE : 0;
        } else if (RF_Bounding_flag && rf_lost_count < RF_LINK_LOST_LIMIT) {
            /* Connected: all 17 yellow solid */
            status_color = RF_LED_YELLOW;
        } else {
            /* Paired but not connected / not paired: LEDs 1-16 yellow, last LED red 2 Hz */
            status_color = led_on ? RF_LED_RED : 0;
        }

        for (uint8_t i = 0; i < RF_LED_COUNT; i++) {
            led_buf[i] = base_color;
        }
        led_buf[RF_LED_LAST] = status_color;

        process_RGB_to_pwm(led_buf, RF_LED_COUNT, Pwmout_buf);
        PWM_DATA_DMA_send(Pwmout_buf, sizeof(Pwmout_buf));
        tmos_start_task(RF_taskID, WS2812_EVENT, MS1_TO_SYSTEM_TIME(RF_LED_REFRESH_MS));
        return (events ^ WS2812_EVENT);
    }

    return 0;
}

/*********************************************************************
 * @fn      RF_ConfigAddress
 *
 * @brief   ���� RF ���ʵ�ַ��Ƶ��ͻص�
 *
 * @return  0 - �ɹ�
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
 * @brief   RF ��ʼ������ȡ���� MAC �����������
 *
 * @return  none
 */
void RF_Init(void)
{
    __attribute__((aligned(4))) uint8_t mac[8] = {0};
    uint32_t saved_mac;

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
    rf_pairing_mode = 0;
    rf_led_blink_state = 0;

    saved_mac = RF_BindFlash_Read();
    if (saved_mac != 0) {
        bounding_mac = saved_mac;
        if (RF_ConfigAddress(bounding_mac) == 0) {
            RF_Bounding_flag = 1;
            rf_lost_count = 100;
            RF_Shut();
            RF_Rx(TX_DATA, 10, 0xFF, 0xFF);
            PRINT("rf restored binding: %x\n", bounding_mac);
        } else {
            RF_ConfigAddress(0x82531829);
        }
    } else {
        RF_ConfigAddress(0x82531829);
    }

    PRINT("rf bind mac: %x\n", keyboard_mac_32);
    tmos_start_task(RF_taskID, SBP_RF_kbd_data_EVT, MS1_TO_SYSTEM_TIME(8));
    tmos_start_task(RF_taskID, WS2812_EVENT, MS1_TO_SYSTEM_TIME(125));
}
/******************************** endfile @ main ******************************/
