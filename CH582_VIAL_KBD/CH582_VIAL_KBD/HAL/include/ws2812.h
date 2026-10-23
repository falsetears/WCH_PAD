/*
 * ws2812.h
 *
 *  Created on: 2024年11月19日
 *      Author: OWNER
 */

#ifndef INCLUDE_WS2812_H_
#define INCLUDE_WS2812_H_

#define WS2812_EVENT              0x0010
#define WS2812_LED_COUNT          17
#define WS2812_PWM_WORDS          (WS2812_LED_COUNT * 24)
#define WS2812_RESET_WORDS        64
#define WS2812_FRAME_WORDS        (WS2812_RESET_WORDS + WS2812_PWM_WORDS)

typedef enum
{
    RGB_Left_flowing_water,
    RGB_Right_flowing_water,

    OTHER, //推挽输出最大20mA

} RGB_move_control;
extern __attribute__((aligned(4))) uint32_t flowing_buf[17];
extern __attribute__((aligned(4))) uint32_t flowing_buf_usb[17];
extern __attribute__((aligned(4))) uint32_t flowing_buf_ble[17];
extern __attribute__((aligned(4))) uint32_t flowing_buf_24[17];
extern __attribute__((aligned(4))) uint32_t Pwmout_buf[WS2812_FRAME_WORDS];
extern void Ws2812_Init(void);
extern void process_RGB_to_pwm(const uint32_t *buf, uint32_t num_elements, uint32_t *output);
extern void Ws2812_move_control(uint32_t *color_buf,RGB_move_control mode,uint32_t len);
extern void PWM_DATA_DMA_send(uint32_t* Pwmout_buf_start,uint32_t len);
#endif /* INCLUDE_WS2812_H_ */
