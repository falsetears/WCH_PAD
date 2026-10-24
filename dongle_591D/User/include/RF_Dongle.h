/********************************** (C) COPYRIGHT *******************************
 * File Name          : rf_phy.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/
#include "ISP.h"
#ifndef CENTRAL_H
#define CENTRAL_H

#ifdef __cplusplus
extern "C" {
#endif



#define SBP_RF_START_DEVICE_EVT    0x0001
#define SBP_RF_PERIODIC_EVT        0x0002
#define SBP_RF_RF_RX_EVT           0x0004
#define SBP_RF_CAPSLOCK_TX_EVT     0x0008
#define SBP_heartbeat_EVT          0x0010
#define SBP_bounding_EVT           0x0020
#define SBP_nobounding_EVT         0x0040
#define SBP_USB_EP3RX_EVT          0x0080
#define SBP_unbind_EVT             0x0100
#define SBP_vial_state_EVT         0x0200
#define RF_BIND_FLASH_ADDR         0x7C00
#define HEARTBEAT_CNT_MAX          20       // ��λ��ms
#define LLE_MODE_ORIGINAL_RX       (0x80)   // �������LLEMODEʱ���ϴ˺꣬����յ�һ�ֽ�Ϊԭʼ���ݣ�ԭ��ΪRSSI��

extern tmosTaskID RFtaskID;
extern uint8_t CAPSLOCK_DATA[2];

extern void RF_Init(uint32_t RF_ADD);
extern void RF_BootInit(void);

#ifdef __cplusplus
}
#endif

#endif
