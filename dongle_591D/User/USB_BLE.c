/********************************** (C) COPYRIGHT *******************************
 * File Name          : USB.c
 * Author             : WCH_yz
 * Version            : V1.1
 * Date               : 2024/4/10
 * Description        : USB驱动源文件
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: GPL-3.0
 *******************************************************************************/

#include "CONFIG.h"
#include "vial_def.h"
#include "USB_BLE.h"
#include "RF_Dongle.h"


#define DevEP0SIZE    0x40
uint8_t USB_VIAL_START = 0;
uint8_t vial_data_count = 0;
tmosTaskID usbTaskID=INVALID_TASK_ID;
// 支持的最大接口数量
#define USB_INTERFACE_MAX_NUM       2
// 接口号的最大值
#define USB_INTERFACE_MAX_INDEX      1

uint8_t key_chang_data[32] = {0};
//uint8_t scan_flag = 0;
uint8_t rgb_flag = 0;
volatile uint8_t vial_out_flag = 0;
// 设备描述符
const uint8_t MyDevDescr[] = {0x12,0x01,0x00,0x02,0x00,0x00,
                              0x00,0x40,0x73,0x92,0x57,0x91,
                              0x00,0x01,0x01,0x02,0x03,0x01};
// 配置描述符
const uint8_t MyCfgDescr[] = {
        0x09,        //   bLength
        0x02,        //   bDescriptorType (Configuration)
        0x5B, 0x00,  //   wTotalLength 91
        0x03,        //   bNumInterfaces 3
        0x01,        //   bConfigurationValue
        0x00,        //   iConfiguration (String Index)
        0xA0,        //   bmAttributes Remote Wakeup
        0xFA,        //   bMaxPower 500mA

        0x09,        //   bLength
        0x04,        //   bDescriptorType (Interface)
        0x00,        //   bInterfaceNumber 0
        0x00,        //   bAlternateSetting
        0x01,        //   bNumEndpoints 1
        0x03,        //   bInterfaceClass
        0x01,        //   bInterfaceSubClass
        0x01,        //   bInterfaceProtocol
        0x00,        //   iInterface (String Index)

        0x09,        //   bLength
        0x21,        //   bDescriptorType (HID)
        0x11, 0x01,  //   bcdHID 1.11
        0x00,        //   bCountryCode
        0x01,        //   bNumDescriptors
        0x22,        //   bDescriptorType[0] (HID)
        0x44, 0x00,  //   wDescriptorLength[0] 68

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x81,        //   bEndpointAddress (IN/D2H)
        0x03,        //   bmAttributes (Interrupt)
        0x08, 0x00,  //   wMaxPacketSize 8
        0x01,        //   bInterval 1 (unit depends on device speed)

        0x09,        //   bLength
        0x04,        //   bDescriptorType (Interface)
        0x01,        //   bInterfaceNumber 1
        0x00,        //   bAlternateSetting
        0x02,        //   bNumEndpoints 2
        0x03,        //   bInterfaceClass
        0x00,        //   bInterfaceSubClass
        0x00,        //   bInterfaceProtocol
        0x00,        //   iInterface (String Index)

        0x09,        //   bLength
        0x21,        //   bDescriptorType (HID)
        0x11, 0x01,  //   bcdHID 1.11
        0x00,        //   bCountryCode
        0x01,        //   bNumDescriptors
        0x22,        //   bDescriptorType[0] (HID)
        0x22, 0x00,  //   wDescriptorLength[0] 34

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x82,        //   bEndpointAddress (IN/D2H)
        0x03,        //   bmAttributes (Interrupt)
        0x20, 0x00,  //   wMaxPacketSize 32
        0x01,        //   bInterval 1 (unit depends on device speed)

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x03,        //   bEndpointAddress (OUT/H2D)
        0x03,        //   bmAttributes (Interrupt)
        0x20, 0x00,  //   wMaxPacketSize 32
        0x01,        //   bInterval 1 (unit depends on device speed)

        0x09,        //   bLength
        0x04,        //   bDescriptorType (Interface)
        0x02,        //   bInterfaceNumber 2
        0x00,        //   bAlternateSetting
        0x01,        //   bNumEndpoints 1
        0x03,        //   bInterfaceClass
        0x00,        //   bInterfaceSubClass
        0x00,        //   bInterfaceProtocol
        0x00,        //   iInterface (String Index)

        0x09,        //   bLength
        0x21,        //   bDescriptorType (HID)
        0x11, 0x01,  //   bcdHID 1.11
        0x00,        //   bCountryCode
        0x01,        //   bNumDescriptors
        0x22,        //   bDescriptorType[0] (HID)
        0x7B, 0x00,  //   wDescriptorLength[0] 123

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x84,        //   bEndpointAddress (IN/D2H)
        0x03,        //   bmAttributes (Interrupt)
        0x20, 0x00,  //   wMaxPacketSize 32
        0x01,        //   bInterval 1 (unit depends on device speed)

};
/* USB速度匹配描述符 */
const uint8_t My_QueDescr[] = {0x0A, 0x06, 0x00, 0x02, 0xFF, 0x00, 0xFF, 0x40, 0x01, 0x00};

/* USB全速模式,其他速度配置描述符 */
uint8_t USB_FS_OSC_DESC[sizeof(MyCfgDescr)] = {
    0x09, 0x07, /* 其他部分通过程序复制 */
};

// 语言描述符
const uint8_t MyLangDescr[] = {0x04, 0x03, 0x09, 0x04};
// 厂家信息
const uint8_t MyManuInfo[] = {0x1C,0x03,0x76,0x00,0x69,0x00,0x61,0x00,0x6C,0x00,
                              0x3A,0x00,0x66,0x00,0x36,0x00,0x34,0x00,0x63,0x00,
                              0x32,0x00,0x62,0x00,0x33,0x00,0x63,0x00};
// 产品信息
const uint8_t MyProdInfo[] = {0x0C, 0x03, 'E', 0, 'B', 0, 'P', 0, 'R', 0, 'F', 0};
/*HID类报表描述符*/
const uint8_t KeyRepDesc[] = {
        0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
        0x09, 0x06,        // Usage (Keyboard)
        0xA1, 0x01,        // Collection (Application)
        0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
        0x19, 0xE0,        //   Usage Minimum (0xE0)
        0x29, 0xE7,        //   Usage Maximum (0xE7)
        0x15, 0x00,        //   Logical Minimum (0)
        0x25, 0x01,        //   Logical Maximum (1)
        0x95, 0x08,        //   Report Count (8)
        0x75, 0x01,        //   Report Size (1)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x95, 0x01,        //   Report Count (1)
        0x75, 0x08,        //   Report Size (8)
        0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
        0x19, 0x00,        //   Usage Minimum (0x00)
        0x29, 0xFF,        //   Usage Maximum (0xFF)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x95, 0x06,        //   Report Count (6)
        0x75, 0x08,        //   Report Size (8)
        0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x05, 0x08,        //   Usage Page (LEDs)
        0x19, 0x01,        //   Usage Minimum (Num Lock)
        0x29, 0x05,        //   Usage Maximum (Kana)
        0x15, 0x00,        //   Logical Minimum (0)
        0x25, 0x01,        //   Logical Maximum (1)
        0x95, 0x05,        //   Report Count (5)
        0x75, 0x01,        //   Report Size (1)
        0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x95, 0x01,        //   Report Count (1)
        0x75, 0x03,        //   Report Size (3)
        0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0xC0,              // End Collection
};


const uint8_t vial_Desc[] = {
        0x06, 0x60, 0xFF,  // Usage Page (Vendor Defined 0xFF60)
        0x09, 0x61,        // Usage (0x61)
        0xA1, 0x01,        // Collection (Application)
        0x09, 0x62,        //   Usage (0x62)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x95, 0x20,        //   Report Count (32)
        0x75, 0x08,        //   Report Size (8)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x09, 0x63,        //   Usage (0x63)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x95, 0x20,        //   Report Count (32)
        0x75, 0x08,        //   Report Size (8)
        0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0xC0,              // End Collection
};
uint8_t vial_Desc_ram[sizeof(vial_Desc)];
volatile uint8_t vial_enabled = 0;
const uint8_t Consumer_Desc[] = {
        0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
        0x09, 0x02,        // Usage (Mouse)
        0xA1, 0x01,        // Collection (Application)
        0x85, 0x02,        //   Report ID (2)
        0x09, 0x01,        //   Usage (Pointer)
        0xA1, 0x00,        //   Collection (Physical)
        0x05, 0x09,        //     Usage Page (Button)
        0x19, 0x01,        //     Usage Minimum (0x01)
        0x29, 0x08,        //     Usage Maximum (0x08)
        0x15, 0x00,        //     Logical Minimum (0)
        0x25, 0x01,        //     Logical Maximum (1)
        0x95, 0x08,        //     Report Count (8)
        0x75, 0x01,        //     Report Size (1)
        0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
        0x09, 0x30,        //     Usage (X)
        0x09, 0x31,        //     Usage (Y)
        0x15, 0x81,        //     Logical Minimum (-127)
        0x25, 0x7F,        //     Logical Maximum (127)
        0x95, 0x02,        //     Report Count (2)
        0x75, 0x08,        //     Report Size (8)
        0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
        0x09, 0x38,        //     Usage (Wheel)
        0x15, 0x81,        //     Logical Minimum (-127)
        0x25, 0x7F,        //     Logical Maximum (127)
        0x95, 0x01,        //     Report Count (1)
        0x75, 0x08,        //     Report Size (8)
        0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
        0x05, 0x0C,        //     Usage Page (Consumer)
        0x0A, 0x38, 0x02,  //     Usage (AC Pan)
        0x15, 0x81,        //     Logical Minimum (-127)
        0x25, 0x7F,        //     Logical Maximum (127)
        0x95, 0x01,        //     Report Count (1)
        0x75, 0x08,        //     Report Size (8)
        0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
        0xC0,              //   End Collection
        0xC0,              // End Collection
        0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
        0x09, 0x80,        // Usage (Sys Control)
        0xA1, 0x01,        // Collection (Application)
        0x85, 0x03,        //   Report ID (3)
        0x19, 0x01,        //   Usage Minimum (Pointer)
        0x2A, 0xB7, 0x00,  //   Usage Maximum (Sys Display LCD Autoscale)
        0x15, 0x01,        //   Logical Minimum (1)
        0x26, 0xB7, 0x00,  //   Logical Maximum (183)
        0x95, 0x01,        //   Report Count (1)
        0x75, 0x10,        //   Report Size (16)
        0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0xC0,              // End Collection
        0x05, 0x0C,        // Usage Page (Consumer)
        0x09, 0x01,        // Usage (Consumer Control)
        0xA1, 0x01,        // Collection (Application)
        0x85, 0x04,        //   Report ID (4)
        0x19, 0x01,        //   Usage Minimum (Consumer Control)
        0x2A, 0xA0, 0x02,  //   Usage Maximum (0x02A0)
        0x15, 0x01,        //   Logical Minimum (1)
        0x26, 0xA0, 0x02,  //   Logical Maximum (672)
        0x95, 0x01,        //   Report Count (1)
        0x75, 0x10,        //   Report Size (16)
        0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0xC0,              // End Collection
};

/**********************************************************/
uint8_t        DevConfig, Ready;
uint8_t        SetupReqCode;
uint16_t       SetupReqLen;
const uint8_t *pDescr;
uint8_t        Report_Value[USB_INTERFACE_MAX_INDEX+1] = {0x00};
uint8_t        Idle_Value[USB_INTERFACE_MAX_INDEX+1] = {0x00};
uint8_t        USB_SleepStatus = 0x00; /* USB睡眠状态 */

Dongle_data_STRUCT Dongle_data;
/*鼠标键盘数据*/
uint8_t HIDMouse[4] = {0x0, 0x0, 0x0, 0x0};
uint8_t HIDKey[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
/******** 用户自定义分配端点RAM ****************************************/
__attribute__((aligned(4))) uint8_t EP0_Databuf[64 + 64 + 64]; //ep0(64)+ep4_out(64)+ep4_in(64)
__attribute__((aligned(4))) uint8_t EP1_Databuf[64 + 64];      //ep1_out(64)+ep1_in(64)
__attribute__((aligned(4))) uint8_t EP2_Databuf[64 + 64];      //ep2_out(64)+ep2_in(64)
__attribute__((aligned(4))) uint8_t EP3_Databuf[64 + 64];      //ep3_out(64)+ep3_in(64)

/*********************************************************************
 * @fn      USB_DevTransProcess
 *
 * @brief   USB 传输处理函数
 *
 * @return  none
 */
__HIGH_CODE
void USB_DevTransProcess(void)
{
    uint8_t len, chtype;
    uint8_t intflag, errflag = 0;

    intflag = R8_USB_INT_FG;
    if(intflag & RB_UIF_TRANSFER)
    {
        if((R8_USB_INT_ST & MASK_UIS_TOKEN) != MASK_UIS_TOKEN) // 非空闲
        {
            switch(R8_USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
            // 分析操作令牌和端点号
            {
                case UIS_TOKEN_IN:
                {
                    switch(SetupReqCode)
                    {
                        case USB_GET_DESCRIPTOR:
                            len = SetupReqLen >= DevEP0SIZE ? DevEP0SIZE : SetupReqLen; // 本次传输长度
                            memcpy(pEP0_DataBuf, pDescr, len);                        /* 加载上传数据 */
                            SetupReqLen -= len;
                            pDescr += len;
                            R8_UEP0_T_LEN = len;
                            R8_UEP0_CTRL ^= RB_UEP_T_TOG; // 翻转
                            break;
                        case USB_SET_ADDRESS:
                            R8_USB_DEV_AD = (R8_USB_DEV_AD & RB_UDA_GP_BIT) | SetupReqLen;
                            R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                            break;

                        case USB_SET_FEATURE:
                            break;

                        default:
                            R8_UEP0_T_LEN = 0; // 状态阶段完成中断或者是强制上传0长度数据包结束控制传输
                            R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                            break;
                    }
                }
                break;

                case UIS_TOKEN_OUT:
                {
                    len = R8_USB_RX_LEN;
                    if(SetupReqCode == 0x09)
                    {
                        PRINT("[%s] Num Lock\t", (pEP0_DataBuf[0] & (1<<0)) ? "*" : " ");
                        PRINT("[%s] Caps Lock\t", (pEP0_DataBuf[0] & (1<<1)) ? "*" : " ");
                        PRINT("[%s] Scroll Lock\n", (pEP0_DataBuf[0] & (1<<2)) ? "*" : " ");
                    }
                }
                break;

                case UIS_TOKEN_OUT | 1:
                {
                    if(R8_USB_INT_ST & RB_UIS_TOG_OK)
                    { // 不同步的数据包将丢弃
                        R8_UEP1_CTRL ^= RB_UEP_R_TOG;
                        len = R8_USB_RX_LEN;
                        DevEP1_OUT_Deal(len);
                    }
                }
                break;

                case UIS_TOKEN_IN | 1:
                    R8_UEP1_CTRL ^= RB_UEP_T_TOG;
                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                    break;

                case UIS_TOKEN_OUT | 2:
                {
                    if(R8_USB_INT_ST & RB_UIS_TOG_OK)
                    { // 不同步的数据包将丢弃
                        R8_UEP2_CTRL ^= RB_UEP_R_TOG;
                        len = R8_USB_RX_LEN;
                        DevEP2_OUT_Deal(len);
                    }
                }
                break;

                case UIS_TOKEN_IN | 2:
                    R8_UEP2_CTRL ^= RB_UEP_T_TOG;
                    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                    break;

                case UIS_TOKEN_OUT | 3:
                {
                    if(R8_USB_INT_ST & RB_UIS_TOG_OK)
                    { // 不同步的数据包将丢弃
                        R8_UEP3_CTRL ^= RB_UEP_R_TOG;
                        len = R8_USB_RX_LEN;
                        DevEP3_OUT_Deal(len);
                    }
                }
                break;

                case UIS_TOKEN_IN | 3:
                    R8_UEP3_CTRL ^= RB_UEP_T_TOG;
                    R8_UEP3_CTRL = (R8_UEP3_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                    break;

                case UIS_TOKEN_OUT | 4:
                {
                    if(R8_USB_INT_ST & RB_UIS_TOG_OK)
                    {
                        R8_UEP4_CTRL ^= RB_UEP_R_TOG;
                        len = R8_USB_RX_LEN;
                        DevEP4_OUT_Deal(len);
                    }
                }
                break;

                case UIS_TOKEN_IN | 4:
                    R8_UEP4_CTRL ^= RB_UEP_T_TOG;
                    R8_UEP4_CTRL = (R8_UEP4_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                    break;

                default:
                    break;
            }
            R8_USB_INT_FG = RB_UIF_TRANSFER;
        }
        if(R8_USB_INT_ST & RB_UIS_SETUP_ACT) // Setup包处理
        {
            R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
            SetupReqLen = pSetupReqPak->wLength;
            SetupReqCode = pSetupReqPak->bRequest;
            chtype = pSetupReqPak->bRequestType;

            len = 0;
            errflag = 0;
            if((pSetupReqPak->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)
            {
                /* 非标准请求 */
                /* 其它请求,如类请求，产商请求等 */
                if(pSetupReqPak->bRequestType & 0x40)
                {
                    /* 厂商请求 */
                }
                else if(pSetupReqPak->bRequestType & 0x20)
                {
                    switch(SetupReqCode)
                    {
                        case DEF_USB_SET_IDLE: /* 0x0A: SET_IDLE */         //主机想设置HID设备特定输入报表的空闲时间间隔
                            Idle_Value[pSetupReqPak->wIndex] = (uint8_t)(pSetupReqPak->wValue>>8);
                            break; //这个一定要有

                        case DEF_USB_SET_REPORT: /* 0x09: SET_REPORT */     //主机想设置HID设备的报表描述符
                            break;

                        case DEF_USB_SET_PROTOCOL: /* 0x0B: SET_PROTOCOL */ //主机想设置HID设备当前所使用的协议
                            Report_Value[pSetupReqPak->wIndex] = (uint8_t)(pSetupReqPak->wValue);
                            break;

                        case DEF_USB_GET_IDLE: /* 0x02: GET_IDLE */         //主机想读取HID设备特定输入报表的当前的空闲比率
                            EP0_Databuf[0] = Idle_Value[pSetupReqPak->wIndex];
                            len = 1;
                            break;

                        case DEF_USB_GET_PROTOCOL: /* 0x03: GET_PROTOCOL */     //主机想获得HID设备当前所使用的协议
                            EP0_Databuf[0] = Report_Value[pSetupReqPak->wIndex];
                            len = 1;
                            break;

                        default:
                            errflag = 0xFF;
                    }
                }
            }
            else /* 标准请求 */
            {
                switch(SetupReqCode)
                {
                    case USB_GET_DESCRIPTOR:
                    {
                        switch(((pSetupReqPak->wValue) >> 8))
                        {
                            case USB_DESCR_TYP_DEVICE:
                            {
                                pDescr = MyDevDescr;
                                len = MyDevDescr[0];
                            }
                            break;

                            case USB_DESCR_TYP_CONFIG:
                            {
                                pDescr = MyCfgDescr;
                                len = MyCfgDescr[2];
                            }
                            break;

                            case USB_DESCR_TYP_HID:
                                switch((pSetupReqPak->wIndex) & 0xff)
                                {
                                    /* 选择接口 */
                                    case 0:
                                        pDescr = (uint8_t *)(&MyCfgDescr[18]);
                                        len = 9;
                                        break;

                                    case 1:
                                        pDescr = (uint8_t *)(&MyCfgDescr[43]);
                                        len = 9;
                                        break;

                                    default:
                                        /* 不支持的字符串描述符 */
                                        errflag = 0xff;
                                        break;
                                }
                                break;

                            case USB_DESCR_TYP_REPORT:
                            {
                                if(((pSetupReqPak->wIndex) & 0xff) == 0) //接口0报表描述符
                                {
                                    pDescr = KeyRepDesc; //数据准备上传
                                    len = sizeof(KeyRepDesc);
                                }

                                else if(((pSetupReqPak->wIndex) & 0xff) == 1) //接口1报表描述符
                                {
                                    pDescr = vial_Desc_ram; //usage page may be switched
                                    len = sizeof(vial_Desc);
                                }
                                else if(((pSetupReqPak->wIndex) & 0xff) == 2) //接口1报表描述符
                                {
                                    pDescr = Consumer_Desc; //数据准备上传
                                    len = sizeof(Consumer_Desc);
                                    Ready = 1; //如果有更多接口，该标准位应该在最后一个接口配置完成后有效
                                }
                                else
                                    len = 0xff; //本程序只有2个接口，这句话正常不可能执行
                            }
                            break;

                            case USB_DESCR_TYP_STRING:
                            {
                                switch((pSetupReqPak->wValue) & 0xff)
                                {
                                    case 1:
                                        pDescr = MyManuInfo;
                                        len = MyManuInfo[0];
                                        break;
                                    case 2:
                                        pDescr = MyProdInfo;
                                        len = MyProdInfo[0];
                                        break;
                                    case 0:
                                        pDescr = MyLangDescr;
                                        len = MyLangDescr[0];
                                        break;
                                    case 3:
                                        pDescr = MyManuInfo;
                                        len = MyManuInfo[0];
                                        break;
                                    default:
                                        errflag = 0xFF; // 不支持的字符串描述符
                                        break;
                                }
                            }
                            break;

                            case 0x06:
                                
                            case 0x07:
                               errflag = 0xff;
                                break;

                            default:
                                errflag = 0xff;
                                break;
                        }
                        if(SetupReqLen > len)
                            SetupReqLen = len; //实际需上传总长度
                        len = (SetupReqLen >= DevEP0SIZE) ? DevEP0SIZE : SetupReqLen;
                        memcpy(pEP0_DataBuf, pDescr, len);
                        pDescr += len;
                    }
                    break;

                    case USB_SET_ADDRESS:
                        SetupReqLen = (pSetupReqPak->wValue) & 0xff;
                        break;

                    case USB_GET_CONFIGURATION:
                        pEP0_DataBuf[0] = DevConfig;
                        if(SetupReqLen > 1)
                            SetupReqLen = 1;
                        break;

                    case USB_SET_CONFIGURATION:
                        DevConfig = (pSetupReqPak->wValue) & 0xff;
                        break;

                    case USB_CLEAR_FEATURE:
                    {
                        if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) // 端点
                        {
                            switch((pSetupReqPak->wIndex) & 0xff)
                            {
                                case 0x83:
                                    R8_UEP3_CTRL = (R8_UEP3_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                                    break;
                                case 0x03:
                                    R8_UEP3_CTRL = (R8_UEP3_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                                    break;
                                case 0x82:
                                    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                                    break;
                                case 0x02:
                                    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                                    break;
                                case 0x81:
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                                    break;
                                case 0x01:
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                                    break;
                                default:
                                    errflag = 0xFF; // 不支持的端点
                                    break;
                            }
                        }
                        else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                        {
                            if(pSetupReqPak->wValue == 1)
                            {
                                USB_SleepStatus &= ~0x01;
                            }
                        }
                        else
                        {
                            errflag = 0xFF;
                        }
                    }
                    break;

                    case USB_SET_FEATURE:
                        if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
                        {
                            /* 端点 */
                            switch(pSetupReqPak->wIndex)
                            {
                                case 0x83:
                                    R8_UEP3_CTRL = (R8_UEP3_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
                                    break;
                                case 0x03:
                                    R8_UEP3_CTRL = (R8_UEP3_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_STALL;
                                    break;
                                case 0x82:
                                    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
                                    break;
                                case 0x02:
                                    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_STALL;
                                    break;
                                case 0x81:
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
                                    break;
                                case 0x01:
                                    R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_STALL;
                                    break;
                                default:
                                    /* 不支持的端点 */
                                    errflag = 0xFF; // 不支持的端点
                                    break;
                            }
                        }
                        else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                        {
                            if(pSetupReqPak->wValue == 1)
                            {
                                /* 设置睡眠 */
                                USB_SleepStatus |= 0x01;
                            }
                        }
                        else
                        {
                            errflag = 0xFF;
                        }
                        break;

                    case USB_GET_INTERFACE:
                        pEP0_DataBuf[0] = 0x00;
                        if(SetupReqLen > 1)
                            SetupReqLen = 1;
                        break;

                    case USB_SET_INTERFACE:
                        break;

                    case USB_GET_STATUS:
                        if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
                        {
                            /* 端点 */
                            pEP0_DataBuf[0] = 0x00;
                            switch(pSetupReqPak->wIndex)
                            {
                                case 0x83:
                                    if((R8_UEP3_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01;
                                    }
                                    break;

                                case 0x03:
                                    if((R8_UEP3_CTRL & (RB_UEP_R_TOG | MASK_UEP_R_RES)) == UEP_R_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01;
                                    }
                                    break;

                                case 0x82:
                                    if((R8_UEP2_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01;
                                    }
                                    break;

                                case 0x02:
                                    if((R8_UEP2_CTRL & (RB_UEP_R_TOG | MASK_UEP_R_RES)) == UEP_R_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01;
                                    }
                                    break;

                                case 0x81:
                                    if((R8_UEP1_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01;
                                    }
                                    break;

                                case 0x01:
                                    if((R8_UEP1_CTRL & (RB_UEP_R_TOG | MASK_UEP_R_RES)) == UEP_R_RES_STALL)
                                    {
                                        pEP0_DataBuf[0] = 0x01;
                                    }
                                    break;
                            }
                        }
                        else if((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                        {
                            pEP0_DataBuf[0] = 0x00;
                            if(USB_SleepStatus)
                            {
                                pEP0_DataBuf[0] = 0x02;
                            }
                            else
                            {
                                pEP0_DataBuf[0] = 0x00;
                            }
                        }
                        pEP0_DataBuf[1] = 0;
                        if(SetupReqLen >= 2)
                        {
                            SetupReqLen = 2;
                        }
                        break;

                    default:
                        errflag = 0xff;
                        break;
                }
            }
            if(errflag == 0xff) // 错误或不支持
            {
                //                  SetupReqCode = 0xFF;
                R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; // STALL
            }
            else
            {
                if(chtype & 0x80) // 上传
                {
                    len = (SetupReqLen > DevEP0SIZE) ? DevEP0SIZE : SetupReqLen;
                    SetupReqLen -= len;
                }
                else
                    len = 0; // 下传
                R8_UEP0_T_LEN = len;
                R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; // 默认数据包是DATA1
            }

            R8_USB_INT_FG = RB_UIF_TRANSFER;
        }
    }
    else if(intflag & RB_UIF_BUS_RST)
    {
        R8_USB_DEV_AD = 0;
        R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_USB_INT_FG = RB_UIF_BUS_RST;
    }
    else if(intflag & RB_UIF_SUSPEND)
    {
        if(R8_USB_MIS_ST & RB_UMS_SUSPEND)
        {
            ;
        } // 挂起
        else
        {
            USB_DevWakeup()  ;
        } // 唤醒
        R8_USB_INT_FG = RB_UIF_SUSPEND;
    }
    else
    {
        R8_USB_INT_FG = intflag;
    }
}

/*******************************************************************************
* Function Name  : USB_ProcessEvent
* Description    : USB处理事件
* Input          : task_id： 任务id, events: USB事件
* Return         : tmosEvents
*******************************************************************************/
tmosEvents USB_ProcessEvent( tmosTaskID task_id, tmosEvents events )
{
  if ( events & START_USB_EVENT )
  {
     PFIC_EnableIRQ( USB_IRQn );

     return events ^ START_USB_EVENT;
  }
//  if ( events & STOP_USB_EVENT )
//  {
//     PFIC_DisableIRQ( USB_IRQn );
//
//     return events ^ STOP_USB_EVENT;
//  }
  if ( events & USB_KEYBOARD_EVENT )
  {
    memcpy(pEP1_IN_DataBuf, &Dongle_data.RXBUFF_key[0], 8);
    DevEP1_IN_Deal( 8 );
    return events ^ USB_KEYBOARD_EVENT;
  }
//  if ( events & USB_test_EVENT )
//  {
//    memcpy(pEP2_IN_DataBuf, &Dongle_data.RXBUFF_keyBIT[0], 16);
//    DevEP3_IN_Deal( 3 );
//    return events ^ USB_test_EVENT;
//  }

  if ( events & USB_VOL_EVENT )
  {
    memcpy(pEP4_IN_DataBuf, &Dongle_data.RXBUFF_vol[0], 3);
    DevEP4_IN_Deal( 3 );
    return events ^ USB_VOL_EVENT;
  }
  if (events & USB_VIAL_EP2_EVENT) {
      memcpy(pEP2_IN_DataBuf, &Dongle_data.TX_VIAL_DATA[0], 32);
      DevEP2_IN_Deal( 32 );
      return events ^ USB_VIAL_EP2_EVENT;
  }
  return 0;
}

/*******************************************************************************
* Function Name  : HAL_USBInit
* Description    : USB初始化
* Input          : None
* Return         : None
*******************************************************************************/
static void USB_ReEnumerate(void)
{
    PFIC_DisableIRQ(USB_IRQn);
    R8_USB_CTRL &= ~RB_UC_DEV_PU_EN;
    R16_PIN_ANALOG_IE &= ~(RB_PIN_USB_IE | RB_PIN_USB_DP_PU);
    R8_UDEV_CTRL &= ~RB_UD_PORT_EN;
    mDelaymS(20);
    USB_DeviceInit();
    PFIC_EnableIRQ(USB_IRQn);
}

void USB_SetVialEnabled(uint8_t en)
{
    if (vial_enabled == en) return;
    vial_enabled = en;
    // 0x60 -> usage page 0xFF60 (VIAL), 0x61 -> 0xFF61 (not VIAL)
    vial_Desc_ram[1] = en ? 0x60 : 0x61;
    USB_ReEnumerate();
}

void HAL_USBInit( void )
{
  memcpy(vial_Desc_ram, vial_Desc, sizeof(vial_Desc));
  vial_Desc_ram[1] = 0x61;  // no keyboard yet: do not look like a VIAL device
  vial_enabled = 0;
  usbTaskID = TMOS_ProcessEventRegister( USB_ProcessEvent );
  pEP0_RAM_Addr = EP0_Databuf;
  pEP1_RAM_Addr = EP1_Databuf;
  pEP2_RAM_Addr = EP2_Databuf;
  pEP3_RAM_Addr = EP3_Databuf;
  USB_DeviceInit();
  tmos_start_task( usbTaskID, START_USB_EVENT, 0 );
}

/*********************************************************************
 * @fn      U2DevWakeup
 *
 * @brief   设备模式唤醒主机
 *
 * @return  none
 */
void USB_DevWakeup(void)
{
//    R16_PIN_ANALOG_IE &= ~(RB_PIN_USB_DP_PU);
//    R8_UDEV_CTRL |= RB_UD_LOW_SPEED;
//    mDelaymS(2);
//    R8_UDEV_CTRL &= ~RB_UD_LOW_SPEED;
//    R16_PIN_ANALOG_IE |= RB_PIN_USB_DP_PU;
}
/*******************************************************************************
* Function Name  : DevEP1_OUT_Deal
* Description    : 端点1数据处理(HID键盘-不使用)
* Input          : None
* Return         : None
*******************************************************************************/
void DevEP1_OUT_Deal( UINT8 l )
{
    PRINT("pEP1_out_Buf:\r\n");
    for (int var = 0; var < l; ++var) {
       PRINT("%x ",pEP1_OUT_DataBuf[var]);
    }
    PRINT("\r\n");
}


/*******************************************************************************
* Function Name  : DevEP2_OUT_Deal
* Description    : 端点2数据处理(HID鼠标-不使用)
* Input          : None
* Return         : None
*******************************************************************************/
void DevEP2_OUT_Deal( UINT8 l )
{
    PRINT("pEP2_out_Buf:\r\n");
    for (int var = 0; var < l; ++var) {
       PRINT("%x ",pEP2_OUT_DataBuf[var]);
    }
    PRINT("\r\n");
}

/*******************************************************************************
* Function Name  : DevEP3_OUT_Deal
* Description    : 端点3数据处理(HID音量控制-不使用)
* Input          : None
* Return         : None
*******************************************************************************/
static uint8_t VIAL_Proxy(const uint8_t *in, uint8_t *out)
{
    uint8_t i;
    memset(out, 0, 32);
    if (in[0] == 0x01) {            /* GET_PROTOCOL_VERSION */
        out[0] = 0x01; out[1] = 0x00; out[2] = 0x09;
        return 1;
    }
    if (in[0] == 0x11) {            /* GET_LAYER_COUNT */
        out[0] = 0x11; out[1] = 1;
        return 1;
    }
    if (in[0] == 0xFE) {            /* VIAL prefix */
        if (in[1] == 0x00) {        /* keyboard id */
            out[0] = 1; out[4] = 1;
        }
        else if (in[1] == 0x01) {   /* definition size */
            out[0] = (uint8_t)(VIAL_DEF_SIZE & 0xFF);
            out[1] = (uint8_t)((VIAL_DEF_SIZE >> 8) & 0xFF);
        }
        else if (in[1] == 0x02) {   /* definition block */
            uint32_t block = (uint32_t)in[2] | ((uint32_t)in[3] << 8) |
                             ((uint32_t)in[4] << 16) | ((uint32_t)in[5] << 24);
            uint32_t base = block * 32u;
            for (i = 0; i < 32; i++)
                out[i] = (base + i < VIAL_DEF_SIZE) ? vial_def_lzma[base + i] : 0;
        }
        else if (in[1] == 0x05) {   /* unlock status */
            out[0] = 1;
        }
        return 1;
    }
    if (in[0] == 0x0C || in[0] == 0x0D || in[0] == 0x0E) {
        out[0] = in[0];
        return 1;
    }
    if (in[0] == 0x02 || in[0] == 0x03 || in[0] == 0x08 || in[0] == 0x09) {
        out[0] = in[0];
        return 1;
    }
    return 0;   /* 0x12 keymap / 0x05 set-key are tunneled to the keyboard */
}

void DevEP3_OUT_Deal(uint8_t l)
{
//    memset(pEP2_IN_DataBuf,0,32);
      PRINT("pEP3_out_Buf:\r\n");
      for (int var = 0; var < l; ++var) {
         PRINT("%x ",pEP3_OUT_DataBuf[var]);
      }
      PRINT("\r\n");
      memcpy(Dongle_data.RX_VIAL_DATA,pEP3_OUT_DataBuf,l);
      {
          uint8_t proxy[32];
          if (VIAL_Proxy(pEP3_OUT_DataBuf, proxy)) {
              memcpy(Dongle_data.TX_VIAL_DATA, proxy, 32);
              tmos_set_event(usbTaskID, USB_VIAL_EP2_EVENT);
              vial_out_flag = 0;
          } else {
              vial_out_flag = 1;
          }
      }
//      tmos_start_task(RFtaskID, SBP_USB_EP3RX_EVT,0);
//    pEP3_OUT_DataBuf;
//    U2DevEP2_IN_Deal(32);
}

/*******************************************************************************
* Function Name  : DevEP4_OUT_Deal
* Description    : 预留
* Input          : None
* Return         : None
*******************************************************************************/
void DevEP4_OUT_Deal( UINT8 l )
{
    PRINT("pEP4_out_Buf:\r\n");
    for (int var = 0; var < l; ++var) {
       PRINT("%x ",pEP4_OUT_DataBuf[var]);
    }
    PRINT("\r\n");
}

__INTERRUPT
__HIGH_CODE
void USB_IRQHandler( void )     //USB中断服务程序,使用寄存器组1
{
    USB_DevTransProcess();
}
