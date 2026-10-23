/*
 * RF_MODE.h
 *
 *  Created on: 2024��12��13��
 *      Author: OWNER
 */

#ifndef INCLUDE_RF_MODE_H_
#define INCLUDE_RF_MODE_H_

#define SBP_RF_START_DEVICE_EVT    1
#define SBP_RF_kbd_data_EVT        2
#define SBP_RF_vol_data_EVT        4
#define SBP_dongle_boot_EVT        8
#define SBP_RF_BIND_EVT            0x0020
#define SBP_RF_REBIND_EVT          0x0040
#define SBP_RF_RF_RX_EVT           0x0100
#define SBP_RF_TX_GUARD_EVT        0x0800
#define SBP_RF_LED_EVT             0x0200
#define RF_BIND_FLASH_ADDR         0x7C00
#define LLE_MODE_ORIGINAL_RX       (0x80) //�������LLEMODEʱ���ϴ˺꣬����յ�һ�ֽ�Ϊԭʼ���ݣ�ԭ��ΪRSSI��

extern void RF_Init(void);

#endif /* INCLUDE_RF_MODE_H_ */
