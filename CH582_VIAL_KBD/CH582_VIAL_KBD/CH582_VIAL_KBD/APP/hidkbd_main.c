/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2024/12/10
 * Description        : 蓝牙键盘应用主函数及任务系统初始化
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "HAL.h"
#include "hiddev.h"
#include "hidkbd.h"
#include "USB_MODE.h"
#include "RF_MODE.h"
#include "scan_key.h"
#include "ws2812.h"
#include "VIAL.h"
/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   主循环
 *
 * @return  none
 */
__HIGH_CODE
__attribute__((noinline))
void Main_Circulation()
{
    while(1)
    {
        TMOS_SystemProcess();
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main(void)
{
    SetSysClock(CLK_SOURCE_PLL_60MHz);

#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_Floating); // PA4 floating for battery divider
#endif

#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
    uint8_t key_mode = 0;
    //vial支持初始化和闭源密码校验，如果检验不通过无法向下运行
    //如果删除校验则键盘不支持vial，读取配列也会失败
    key_mode = vial_init();
    PRINT("key_mode=0x%02X\r\n", key_mode);
    mDelaymS(20);          // 等串口发完模式值，避免被后续复位截断
    Ws2812_Init();
    Scan_init();

    // 数据Flash中的模式值非法时默认USB，避免落入SYS_ResetExecute复位循环
    if (key_mode != 0x0B && key_mode != 0xBE && key_mode != 0x24)
    {
        key_mode = 0x0B;
    }

    if (key_mode==0x0B)
    {
        process_RGB_to_pwm(flowing_buf_usb, 17, &Pwmout_buf[WS2812_RESET_WORDS]);
        PWM_DATA_DMA_send(Pwmout_buf, WS2812_FRAME_WORDS);
        USB_INIT();
    }
    else if (key_mode==0xBE) {  //BLE MODE
        process_RGB_to_pwm(flowing_buf_ble, 17, &Pwmout_buf[WS2812_RESET_WORDS]);
        PWM_DATA_DMA_send(Pwmout_buf, WS2812_FRAME_WORDS);
        PWR_DCDCCfg(ENABLE);
        CH58X_BLEInit();
        HAL_Init();
        GAPRole_PeripheralInit();
        HidDev_Init();
        HidEmu_Init();
    }
    else if (key_mode==0x24) {  //2.4G mode
        process_RGB_to_pwm(flowing_buf_24, 17, &Pwmout_buf[WS2812_RESET_WORDS]);
        PWM_DATA_DMA_send(Pwmout_buf, WS2812_FRAME_WORDS);
        PWR_DCDCCfg(ENABLE);
        CH58X_BLEInit();
        HAL_Init();
        RF_RoleInit();
        RF_Init();
    }
    else {
        SYS_ResetExecute();
    }
    Main_Circulation();
}

/*******************************************************************************
* Function Name  : GPIOA_IRQHandler
* Description    : GPIOA外部中断,低电平触发
* Input          : None
* Return         : None
*******************************************************************************/
__INTERRUPT
__HIGH_CODE
void GPIOA_IRQHandler(void)
{
   GPIOA_ClearITFlagBit(GPIO_Pin_5);
   SYS_ResetExecute();
}



/******************************** endfile @ main ******************************/
