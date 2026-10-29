/********************************** (C) COPYRIGHT *******************************
 * File Name          : ws2812.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2024年11月19日
 * Description        : ws2812驱动层
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "config.h"
#include "ws2812.h"

__attribute__((aligned(4))) uint32_t flowing_buf[17]={
//        0x00000F,0x000F00,0x0f0000,0x000F00,0x000808,
//        0x00000F,0x000F00,0x0f0000,0x000F00,0x000f00,
        0x00000F,0x000F00,0x0f0000,0x000F00,0x000808,
        0x0c0909,0x060201,
};
__attribute__((aligned(4))) uint32_t flowing_buf_usb[17]={
        0x000800,0x000800,0x000800,0x000800,0x000800,
        0x000800,0x000800,0x000800,0x000800,0x000800,
        0x000800,0x000800,0x000800,0x000800,0x000800,
        0x000800,0x000800,
};
__attribute__((aligned(4))) uint32_t flowing_buf_ble[17]={
        0x000008,0x000008,0x000008,0x000008,0x000008,
        0x000008,0x000008,0x000008,0x000008,0x000008,
        0x000008,0x000008,0x000008,0x000008,0x000008,
        0x000008,0x000008,
};
__attribute__((aligned(4))) uint32_t flowing_buf_24[17]={
        0x080800,0x080800,0x080800,0x080800,0x080800,
        0x080800,0x080800,0x080800,0x080800,0x080800,
        0x080800,0x080800,0x080800,0x080800,0x080800,
        0x080800,0x080800,
};
__attribute__((aligned(4))) uint32_t Pwmout_buf[WS2812_FRAME_WORDS]={0};

/*********************************************************************
 * @fn      Ws2812_Init
 *
 * @brief   ws2812初始化，pwm+DMA控制，中断开启。
 *
 * @param   none
 *
 * @return  none
 */
void Ws2812_Init(void)
{
    uint8_t i;
    GPIOA_ModeCfg(GPIO_Pin_11, GPIO_ModeOut_PP_5mA);
    GPIOA_ResetBits(GPIO_Pin_11);//pwm管脚初始化
    GPIOB_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    GPIOB_ResetBits(GPIO_Pin_9);  //开启背光
    //这里选择rgb的控制周期为1.2us
    //至于为什么是1.2us可参考ws2812手册数据传输时间章节
    PRINT("TMR2 DMA PWM\n");
    TMR2_PWMInit(High_Level, PWM_Times_1);
    TMR2_PWMCycleCfg(72); // 主频是60Mhz 每秒震荡60M次 震荡60次为1微秒(60*1.2=72)
    //TMR2_ClearITFlag(TMR1_2_IT_DMA_END);
    //TMR2_ITCfg(ENABLE, TMR1_2_IT_DMA_END);
    //PFIC_EnableIRQ(TMR2_IRQn);
    vialrgb_init();
}

/*********************************************************************
 * @fn      process_RGB_to_pwm
 *
 * @brief   功能函数，将RGB数据缓冲区的数据转换成pwm+dma数据
 *
 * @param           *buf : RGB color数据缓冲区起始地址
 *          num_elements : 数据长
 *               *output : pwm输出数据缓冲区起始地址,该缓冲区大小应为颜色缓冲区大小的24倍
 *
 * @return  none
 */
__HIGH_CODE
void process_RGB_to_pwm(const uint32_t *buf, uint32_t num_elements, uint32_t *output) {
    uint32_t output_index = 0;
    uint32_t byte_accumulator = 0;
    int bit_count = 0;
    for (uint32_t i = 0; i < num_elements; ++i) {
        uint32_t buf_data = buf[i];
        uint32_t current_word_1 = (buf_data&0xff0000)>>16;
        uint32_t current_word_2 = (buf_data&0xff00)>>8;
        uint32_t current_word_3 =  buf_data&0xff;  //调整RGB顺序
        uint32_t current_word_4 = (current_word_2<<16)+(current_word_1<<8)+current_word_3;
        uint32_t current_word = current_word_4 & 0x00ffffff; // 取低24位
        for (int j = 0; j < 24; ++j) {
            uint32_t bit = (current_word << j) & 0x800000; // 高位在前
            output[output_index] = bit ? 0x000024 : 0x000014;
            output_index++;
        }
    }
}
/*********************************************************************
 * @fn      Ws2812_move_control
 *
 * @brief   RGB动态控制
 *
 * @param   *pwm_buf - 初始颜色数据缓冲区
 * @param       mode - 动态类型
 * @param        len - 控制长
 *
 * @return  none
 */
void Ws2812_move_control(uint32_t *color_buf,RGB_move_control mode,uint32_t len)
{
    switch (mode) {
        case 0://流水灯模式,左移
        {
            uint32_t firstElement = color_buf[0];
               for (int i = 0; i < len - 1; i++) {
                   color_buf[i] = color_buf[i + 1];
               }
               color_buf[len - 1] = firstElement;
        }
           break;
        case 1://流水灯模式,右移
        {
            int lastElement = color_buf[len - 1];
            for (int i = len - 1; i > 0; i--) {
                color_buf[i] = color_buf[i - 1];
            }
            color_buf[0] = lastElement;
        }
            break;
        default:
            break;
    }
}

/*********************************************************************
 * @fn      PWM_DATA_DMA_send
 *
 * @brief   PWM通过DMA将数据下发到RGB
 *
 * @param   *Pwmout_buf_start - 待下发数据缓冲区起始地址
 * @param   len               - 待下发数据长
 *
 * @return  none
 */
void PWM_DATA_DMA_send(uint32_t* Pwmout_buf_start,uint32_t len)
{
    // Configure DMA before starting the timer.
    // If the timer is started first, an RF interrupt can land before the
    // DMA is enabled and the timer emits a few default-duty bits, which
    // shifts the whole WS2812 frame (yellow->purple/blue, red->blue, ...).
    //以下DMA配置与输出操作最好使用寄存器操作，库函数有问题
    R16_TMR2_DMA_BEG = (uint32_t)Pwmout_buf_start;  //DMA起始地址
    R16_TMR2_DMA_END = (uint32_t)(Pwmout_buf_start+len);    //DMA结束地址
    R8_TMR2_CTRL_MOD |= RB_TMR_OUT_EN;              //TMR2_PWMEnable();
    R8_TMR2_CTRL_DMA = RB_TMR_DMA_ENABLE;           //DMA输出使能
    TMR2_ClearITFlag(TMR1_2_IT_DMA_END);

    TMR2_Enable();

    // Send synchronously. While the WS2812 frame is on the wire the RF
    // task must not run RF_Shut()/RF_Tx; 408 words take about 0.5 ms.
    for (volatile uint32_t wait = 0; wait < 30000; wait++) {
        if (TMR2_GetITFlag(TMR1_2_IT_DMA_END)) {
            break;
        }
    }

    TMR2_ClearITFlag(TMR1_2_IT_DMA_END);
    TMR2_Disable();
    TMR2_PWMDisable();
    GPIOA_ResetBits(GPIO_Pin_11);
}

/*********************************************************************
 * @fn      TMR2_IRQHandler
 *
 * @brief   TMR2中断函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void TMR2_IRQHandler(void)
{
    if(TMR2_GetITFlag(TMR0_3_IT_CYC_END))
    {
        TMR2_ClearITFlag(TMR0_3_IT_CYC_END);
        /* 计数器计满，硬件自动清零，重新开始计数 */
        /* 用户可自行添加需要的处理 */
    }
    if(TMR2_GetITFlag(TMR1_2_IT_DMA_END))
    {
        TMR2_ClearITFlag(TMR1_2_IT_DMA_END);
        TMR2_Disable();
        TMR2_PWMDisable();
        GPIOA_ResetBits(GPIO_Pin_11);
        TMR2_ITCfg(DISABLE, TMR1_2_IT_DMA_END);
        PFIC_DisableIRQ(TMR2_IRQn);
        /* DMA 结束 */
    }
}

/* ---- VIALRGB effect engine ---- */

vialrgb_config_t vialrgb_cfg = {2, 128, 0, 255, 255, 1};
uint8_t vialrgb_active = 0;
static uint16_t vialrgb_phase = 0;

static const uint8_t led_row[17] = {0,0,0,0, 1,1,1, 2,2,2, 1, 3,3,3, 4,4, 3};
static const uint8_t led_col[17] = {0,1,2,3, 0,1,2, 0,1,2, 3, 0,1,2, 0,2, 3};

#define VIALRGB_FLASH_ADDR  0x7E00
static volatile uint8_t vialrgb_save_pending = 0;

static void vialrgb_flash_save(void)
{
    __attribute__((aligned(4))) uint32_t buf[2] = {0};
    buf[0] = (uint32_t)(vialrgb_cfg.mode & 0xFF) | ((uint32_t)((vialrgb_cfg.mode >> 8) & 0xFF) << 8) |
             ((uint32_t)vialrgb_cfg.speed << 16) | ((uint32_t)vialrgb_cfg.hue << 24);
    buf[1] = (uint32_t)vialrgb_cfg.sat | ((uint32_t)vialrgb_cfg.val << 8) |
             ((uint32_t)vialrgb_cfg.enabled << 16) | (0xFFu << 24);
    EEPROM_ERASE(VIALRGB_FLASH_ADDR, 256);
    EEPROM_WRITE(VIALRGB_FLASH_ADDR, buf, 8);
}

static void vialrgb_flash_load(void)
{
    __attribute__((aligned(4))) uint32_t buf[2] = {0};
    EEPROM_READ(VIALRGB_FLASH_ADDR, buf, 8);
    if (buf[1] == 0xFFFFFFFF || (buf[1] >> 24) != 0xFF) return;
    vialrgb_cfg.mode = (uint16_t)(buf[0] & 0xFFFF);
    vialrgb_cfg.speed = (uint8_t)((buf[0] >> 16) & 0xFF);
    vialrgb_cfg.hue = (uint8_t)((buf[0] >> 24) & 0xFF);
    vialrgb_cfg.sat = (uint8_t)(buf[1] & 0xFF);
    vialrgb_cfg.val = (uint8_t)((buf[1] >> 8) & 0xFF);
    vialrgb_cfg.enabled = (uint8_t)((buf[1] >> 16) & 0xFF);
    if (vialrgb_cfg.mode >= VIALRGB_EFFECT_MAX) vialrgb_cfg.mode = VIALRGB_EFFECT_SOLID;
    vialrgb_active = 1;
}

void vialrgb_flash_flush(void)
{
    if (!vialrgb_save_pending) return;
    vialrgb_save_pending = 0;
    vialrgb_flash_save();
}

uint32_t hsv_to_rgb32(uint8_t h, uint8_t s, uint8_t v)
{
    uint32_t r, g, b;
    uint32_t region = h / 43;
    uint32_t rem = (h - region * 43) * 255 / 43;
    uint32_t p = (uint32_t)v * (255 - s) / 255;
    uint32_t q = (uint32_t)v * (255 - (uint32_t)s * rem / 255) / 255;
    uint32_t t = (uint32_t)v * (255 - (uint32_t)s * (255 - rem) / 255) / 255;
    switch (region) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return (r << 16) | (g << 8) | b;
}

void vialrgb_init(void)
{
    vialrgb_cfg.mode = VIALRGB_EFFECT_SOLID;
    vialrgb_cfg.speed = 128;
    vialrgb_cfg.hue = 0;
    vialrgb_cfg.sat = 255;
    vialrgb_cfg.val = 255;
    vialrgb_cfg.enabled = 1;
    vialrgb_active = 0;
    vialrgb_phase = 0;
    vialrgb_flash_load();
}

void vialrgb_set_mode(uint16_t mode, uint8_t speed, uint8_t hue, uint8_t sat, uint8_t val)
{
    if (mode >= VIALRGB_EFFECT_MAX) mode = VIALRGB_EFFECT_SOLID;
    vialrgb_cfg.mode = mode;
    vialrgb_cfg.speed = speed;
    vialrgb_cfg.hue = hue;
    vialrgb_cfg.sat = sat;
    vialrgb_cfg.val = val;
    vialrgb_cfg.enabled = 1;
    vialrgb_active = 1;
    vialrgb_phase = 0;
    vialrgb_save_pending = 1;
}

static uint8_t breathe_curve(uint8_t phase)
{
    int16_t d = (int16_t)phase - 128;
    if (d < 0) d = -d;
    uint16_t c = (uint16_t)(128 - d) * (128 - d) / 128 * 2;
    return (uint8_t)(c > 255 ? 255 : c);
}

void vialrgb_tick(uint32_t *led_buf)
{
    uint8_t i;
    uint8_t h = vialrgb_cfg.hue;
    uint8_t s = vialrgb_cfg.sat;
    uint8_t v = vialrgb_cfg.val;

    vialrgb_flash_flush();
    if (!vialrgb_cfg.enabled || v == 0 || vialrgb_cfg.mode == VIALRGB_EFFECT_DISABLE) {
        for (i = 0; i < 17; i++) led_buf[i] = 0;
        return;
    }

    vialrgb_phase += (uint8_t)(vialrgb_cfg.speed / 4 + 1);

    switch (vialrgb_cfg.mode) {
        case VIALRGB_EFFECT_DIRECT:
        case VIALRGB_EFFECT_SOLID:
        case VIALRGB_EFFECT_ALPHAS_MODS:
        case VIALRGB_EFFECT_SOLID_REACTIVE_SIMPLE:
        case VIALRGB_EFFECT_SOLID_REACTIVE:
        case VIALRGB_EFFECT_SOLID_REACTIVE_WIDE:
        case VIALRGB_EFFECT_SOLID_REACTIVE_MULTIWIDE:
        case VIALRGB_EFFECT_SOLID_REACTIVE_CROSS:
        case VIALRGB_EFFECT_SOLID_REACTIVE_MULTICROSS:
        case VIALRGB_EFFECT_SOLID_REACTIVE_NEXUS:
        case VIALRGB_EFFECT_SOLID_REACTIVE_MULTINEXUS:
        case VIALRGB_EFFECT_SOLID_SPLASH:
        case VIALRGB_EFFECT_SOLID_MULTISPLASH:
            {
                uint32_t c = hsv_to_rgb32(h, s, v);
                for (i = 0; i < 17; i++) led_buf[i] = c;
            }
            break;

        case VIALRGB_EFFECT_BREATHING:
        case VIALRGB_EFFECT_HUE_BREATHING:
            {
                uint8_t bv = (uint8_t)((uint16_t)v * breathe_curve(vialrgb_phase & 0xFF) / 255);
                uint32_t c = hsv_to_rgb32(h, s, bv);
                for (i = 0; i < 17; i++) led_buf[i] = c;
            }
            break;

        case VIALRGB_EFFECT_GRAD_UD:
            for (i = 0; i < 17; i++)
                led_buf[i] = hsv_to_rgb32((uint8_t)(h + led_row[i] * 51), s, v);
            break;

        case VIALRGB_EFFECT_GRAD_LR:
            for (i = 0; i < 17; i++)
                led_buf[i] = hsv_to_rgb32((uint8_t)(h + led_col[i] * 64), s, v);
            break;

        case VIALRGB_EFFECT_BAND_SAT:
        case VIALRGB_EFFECT_BAND_VAL:
        case VIALRGB_EFFECT_BAND_PINWHEEL_SAT:
        case VIALRGB_EFFECT_BAND_PINWHEEL_VAL:
        case VIALRGB_EFFECT_BAND_SPIRAL_SAT:
        case VIALRGB_EFFECT_BAND_SPIRAL_VAL:
            {
                uint8_t bp = vialrgb_phase % 17;
                for (i = 0; i < 17; i++) {
                    int8_t dist = (int8_t)(i - bp);
                    if (dist < 0) dist = -dist;
                    if (dist > 8) dist = 16 - dist;
                    uint8_t bv = (dist < 3) ? v : (uint8_t)(v / 4);
                    led_buf[i] = hsv_to_rgb32(h, s, bv);
                }
            }
            break;

        case VIALRGB_EFFECT_CYCLE_ALL:
            for (i = 0; i < 17; i++)
                led_buf[i] = hsv_to_rgb32((uint8_t)(h + vialrgb_phase), s, v);
            break;

        case VIALRGB_EFFECT_CYCLE_LR:
            for (i = 0; i < 17; i++)
                led_buf[i] = hsv_to_rgb32((uint8_t)(h + led_col[i] * 64 + vialrgb_phase), s, v);
            break;

        case VIALRGB_EFFECT_CYCLE_UD:
            for (i = 0; i < 17; i++)
                led_buf[i] = hsv_to_rgb32((uint8_t)(h + led_row[i] * 51 + vialrgb_phase), s, v);
            break;

        case VIALRGB_EFFECT_RAINBOW_CHEVRON:
        case VIALRGB_EFFECT_CYCLE_PINWHEEL:
        case VIALRGB_EFFECT_CYCLE_SPIRAL:
            for (i = 0; i < 17; i++)
                led_buf[i] = hsv_to_rgb32((uint8_t)(h + i * 15 + vialrgb_phase), s, v);
            break;

        case VIALRGB_EFFECT_CYCLE_OI:
        case VIALRGB_EFFECT_CYCLE_OI_DUAL:
            for (i = 0; i < 17; i++) {
                uint8_t dist = (led_row[i] > 2) ? led_row[i] - 2 : 2 - led_row[i];
                led_buf[i] = hsv_to_rgb32((uint8_t)(h + dist * 51 + vialrgb_phase), s, v);
            }
            break;

        case VIALRGB_EFFECT_DUAL_BEACON:
        case VIALRGB_EFFECT_RAINBOW_BEACON:
            {
                uint8_t p1 = vialrgb_phase % 17;
                uint8_t p2 = (16 - p1) % 17;
                for (i = 0; i < 17; i++) {
                    uint8_t bv = 0;
                    if (i == p1 || i == p2) bv = v;
                    else if (i == (p1+1)%17 || i == (p1+16)%17 ||
                             i == (p2+1)%17 || i == (p2+16)%17) bv = v/2;
                    led_buf[i] = hsv_to_rgb32(h, s, bv);
                }
            }
            break;

        case VIALRGB_EFFECT_RAINBOW_PINWHEELS:
        case VIALRGB_EFFECT_RAIN_DROPS:
        case VIALRGB_EFFECT_JELLYBEAN_RAIN:
        case VIALRGB_EFFECT_DIGITAL_RAIN:
        case VIALRGB_EFFECT_PIXEL_RAIN:
            for (i = 0; i < 17; i++) {
                uint8_t ph = (uint8_t)(vialrgb_phase + led_row[i] * 64);
                uint8_t bv = (uint8_t)((uint16_t)v * breathe_curve(ph) / 255);
                led_buf[i] = hsv_to_rgb32((uint8_t)(h + i * 15), s, bv);
            }
            break;

        case VIALRGB_EFFECT_HUE_PENDULUM:
        case VIALRGB_EFFECT_HUE_WAVE:
            for (i = 0; i < 17; i++) {
                uint8_t ph = (uint8_t)(vialrgb_phase + led_col[i] * 64);
                uint8_t bv = (uint8_t)((uint16_t)v * breathe_curve(ph) / 255);
                led_buf[i] = hsv_to_rgb32((uint8_t)(h + led_col[i] * 32), s, bv);
            }
            break;

        case VIALRGB_EFFECT_TYPING_HEATMAP:
        case VIALRGB_EFFECT_SPLASH:
        case VIALRGB_EFFECT_MULTISPLASH:
        case VIALRGB_EFFECT_PIXEL_FRACTAL:
            {
                uint32_t c = hsv_to_rgb32(h, s, v);
                for (i = 0; i < 17; i++) led_buf[i] = c;
            }
            break;

        default:
            for (i = 0; i < 17; i++) led_buf[i] = 0;
            break;
    }
}
