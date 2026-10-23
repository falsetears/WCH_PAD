/********************************** (C) COPYRIGHT *******************************
 * File Name          : USB.h
 * Author             : ChnMasterOG
 * Version            : V1.0
 * Date               : 2021/11/17
 * Description        : USB驱动头文件
 *******************************************************************************/

#ifndef __USB_H
    #define __USB_H

    #include "CH59x_common.h"

    #define DevEP0SIZE      0x40
    #define NumsLockLED     0x01
    #define CapsLockLED     0x02
    #define ScrollLockLED   0x04

    #define Volume_Play     (1 << 0)
    #define Volume_Next     (1 << 1)
    #define Volume_Prev     (1 << 2)
    #define Volume_Incr     (1 << 3)
    #define Volume_Decr     (1 << 4)
    #define Volume_Mute     (1 << 5)
    #define Volume_Pause    (1 << 6)
    #define Volume_Stop     (1 << 7)

    #define USB_KEYBIT_EVENT       0x0001
    #define USB_KEYBOARD_EVENT     0x0002
    #define USB_VOL_EVENT          0x0004
    #define START_USB_EVENT        0x0008
    #define USB_CONNECT_EVENT      0x0010
    #define USB_VIAL_EP2_EVENT     0x0020
    #define USB_TEST_EVENT         0x1000
    #define STOP_USB_EVENT         0x2000

    #define USB_ERR_LENGTH        0x01
    #define USB_DAT_INVALID       0x02
    #define USB_ERR_UNKNOWN       0x10

    struct HIDdata {
        UINT8 label;
        union {
            UINT8 Mouse[4];
            UINT8 Keyboard[8];
            UINT8 Volume[1];
            UINT8 data[8];
        };
     };

    typedef struct
    {
        uint8_t RXBUFF_key[8];  //USB KBD DATA
        uint8_t RXBUFF_keyBIT[17];  //USB BITKBD DATA
        uint8_t RXBUFF_MOUSE[4];  //USB MOUSE DATA
        uint8_t RXBUFF_vol[1];  //USB KBD DATA
        uint8_t led[2];
        uint8_t RX_VIAL_DATA[32];
        uint8_t TX_VIAL_DATA[32];
    } Dongle_data_STRUCT;


    extern struct HIDdata HID_dat;

    extern const UINT8 MyDevDescr[];
    extern const UINT8 MyCfgDescr[];
    extern const UINT8 MyLangDescr[];
    extern const UINT8 MyManuInfo[];
    extern const UINT8 MyProdInfo[] ;
    extern const UINT8 KeyRepDesc[];
    extern const UINT8 MouseRepDesc[];
    extern UINT8 DevConfig;
    extern UINT8 SetupReqCode;
    extern UINT16 SetupReqLen;
    extern const UINT8 *pDescr;

    extern tmosTaskID usbTaskID;
    extern BOOL USB_Ready;
    extern BOOL USB_CapsLock_LEDOn;

    void HAL_USBInit( void );
    void USB_DevWakeup(void);
#endif
