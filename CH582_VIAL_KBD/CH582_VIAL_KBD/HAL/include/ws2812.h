/*
 * ws2812.h
 *
 *  Created on: 2024Äê11ÔÂ19ÈÕ
 *      Author: OWNER
 */

#ifndef INCLUDE_WS2812_H_
#define INCLUDE_WS2812_H_

#include <stdint.h>

#define WS2812_EVENT              0x0010
#define WS2812_LED_COUNT          17
#define WS2812_PWM_WORDS          (WS2812_LED_COUNT * 24)
#define WS2812_RESET_WORDS        64
#define WS2812_FRAME_WORDS        (WS2812_RESET_WORDS + WS2812_PWM_WORDS)

typedef enum
{
    RGB_Left_flowing_water,
    RGB_Right_flowing_water,

    OTHER,

} RGB_move_control;

/* ---- VIALRGB effect engine (IDs match VIAL GUI VIALRGB_EFFECTS) ---- */

#define VIALRGB_EFFECT_DISABLE          0
#define VIALRGB_EFFECT_DIRECT           1
#define VIALRGB_EFFECT_SOLID            2
#define VIALRGB_EFFECT_ALPHAS_MODS      3
#define VIALRGB_EFFECT_GRAD_UD          4
#define VIALRGB_EFFECT_GRAD_LR          5
#define VIALRGB_EFFECT_BREATHING        6
#define VIALRGB_EFFECT_BAND_SAT         7
#define VIALRGB_EFFECT_BAND_VAL         8
#define VIALRGB_EFFECT_BAND_PINWHEEL_SAT 9
#define VIALRGB_EFFECT_BAND_PINWHEEL_VAL 10
#define VIALRGB_EFFECT_BAND_SPIRAL_SAT  11
#define VIALRGB_EFFECT_BAND_SPIRAL_VAL  12
#define VIALRGB_EFFECT_CYCLE_ALL        13
#define VIALRGB_EFFECT_CYCLE_LR         14
#define VIALRGB_EFFECT_CYCLE_UD         15
#define VIALRGB_EFFECT_RAINBOW_CHEVRON  16
#define VIALRGB_EFFECT_CYCLE_OI         17
#define VIALRGB_EFFECT_CYCLE_OI_DUAL    18
#define VIALRGB_EFFECT_CYCLE_PINWHEEL   19
#define VIALRGB_EFFECT_CYCLE_SPIRAL     20
#define VIALRGB_EFFECT_DUAL_BEACON      21
#define VIALRGB_EFFECT_RAINBOW_BEACON   22
#define VIALRGB_EFFECT_RAINBOW_PINWHEELS 23
#define VIALRGB_EFFECT_RAIN_DROPS       24
#define VIALRGB_EFFECT_JELLYBEAN_RAIN   25
#define VIALRGB_EFFECT_HUE_BREATHING    26
#define VIALRGB_EFFECT_HUE_PENDULUM     27
#define VIALRGB_EFFECT_HUE_WAVE         28
#define VIALRGB_EFFECT_TYPING_HEATMAP   29
#define VIALRGB_EFFECT_DIGITAL_RAIN     30
#define VIALRGB_EFFECT_SOLID_REACTIVE_SIMPLE 31
#define VIALRGB_EFFECT_SOLID_REACTIVE   32
#define VIALRGB_EFFECT_SOLID_REACTIVE_WIDE 33
#define VIALRGB_EFFECT_SOLID_REACTIVE_MULTIWIDE 34
#define VIALRGB_EFFECT_SOLID_REACTIVE_CROSS 35
#define VIALRGB_EFFECT_SOLID_REACTIVE_MULTICROSS 36
#define VIALRGB_EFFECT_SOLID_REACTIVE_NEXUS 37
#define VIALRGB_EFFECT_SOLID_REACTIVE_MULTINEXUS 38
#define VIALRGB_EFFECT_SPLASH           39
#define VIALRGB_EFFECT_MULTISPLASH      40
#define VIALRGB_EFFECT_SOLID_SPLASH     41
#define VIALRGB_EFFECT_SOLID_MULTISPLASH 42
#define VIALRGB_EFFECT_PIXEL_RAIN       43
#define VIALRGB_EFFECT_PIXEL_FRACTAL    44
#define VIALRGB_EFFECT_MAX              45

typedef struct {
    uint16_t mode;
    uint8_t  speed;
    uint8_t  hue;
    uint8_t  sat;
    uint8_t  val;
    uint8_t  enabled;
} vialrgb_config_t;

extern vialrgb_config_t vialrgb_cfg;
extern uint8_t vialrgb_active;

extern void vialrgb_init(void);
extern void vialrgb_set_mode(uint16_t mode, uint8_t speed, uint8_t hue, uint8_t sat, uint8_t val);
extern void vialrgb_tick(uint32_t *led_buf);
extern uint32_t hsv_to_rgb32(uint8_t h, uint8_t s, uint8_t v);

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
