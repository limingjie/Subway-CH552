#include <stdint.h>
#include <ch554.h>
#include <debug.h>
#include "bitbang.h"

// Define PINs
#define BTN_CLOSE_PIN 4  // P1.4
#define BTN_RUN_PIN   7  // P1.7
#define LED_PIN       0  // P3.0
#define LED_EN_PIN    1  // P3.1
#define LATCH_PIN     2  // P3.2

// Define bits
SBIT(LED, 0xB0, LED_PIN);        // P3.0
SBIT(LED_EN, 0xB0, LED_EN_PIN);  // P3.1
SBIT(LATCH, 0xB0, LATCH_PIN);    // P3.2

// WS2812 data
#define LED_COUNT (255)
__xdata uint8_t led_data[LED_COUNT * 3];

void rgbPattern()
{
    static uint8_t brightness = 5;
    static uint8_t color      = 0;

    uint8_t i;

    // Fill in some data
    for (i = 0; i < LED_COUNT; i++)
    {
        uint8_t seq = (color + i) % 7;
        // LED ring is GRB
        led_data[i * 3 + 1] = ((seq == 0) || (seq == 3) || (seq == 5) || (seq == 6)) ? (brightness) : 0;
        led_data[i * 3 + 0] = ((seq == 1) || (seq == 3) || (seq == 4) || (seq == 6)) ? (brightness) : 0;
        led_data[i * 3 + 2] = ((seq == 2) || (seq == 4) || (seq == 5) || (seq == 6)) ? (brightness) : 0;
    }

    color = (color + 1) % 7;  // 3 for RGB, 7 for RGB???W
}

void main()
{
    uint8_t previousP1 = P1;
    uint8_t ledRun     = 0;

    CfgFsys();
    mDelaymS(5);

    // Configure pin 3.2 as GPIO output
    P3_MOD_OC &= ~(1 << LATCH_PIN);
    P3_DIR_PU |= (1 << LATCH_PIN);
    LATCH = 0;  // Latch power on

    // Configure pin 3.1 as GPIO output
    P3_MOD_OC &= ~(1 << LED_EN_PIN);
    P3_DIR_PU |= (1 << LED_EN_PIN);
    LED_EN = 1;  // Disable LED

    // Configure pin 3.0 as GPIO output
    P3_MOD_OC &= ~(1 << LED_PIN);
    P3_DIR_PU |= (1 << LED_PIN);

    // Configure pin 1.7 as GPIO input
    P1_MOD_OC &= ~(1 << BTN_RUN_PIN);
    P1_DIR_PU &= ~(1 << BTN_RUN_PIN);

    // Configure pin 1.4 as GPIO input
    P1_MOD_OC &= ~(1 << BTN_CLOSE_PIN);
    P1_DIR_PU &= ~(1 << BTN_CLOSE_PIN);

    while (1)
    {
        // BTN_RUN_PIN Released
        if ((previousP1 & (1 << BTN_RUN_PIN)) == 0 && (P1 & (1 << BTN_RUN_PIN)) != 0)
        {
            ledRun = (ledRun == 0) ? 1 : 0;
            LED_EN = ledRun ? 0 : 1;
            mDelaymS(50);
        }

        if ((previousP1 & (1 << BTN_CLOSE_PIN)) == 0 && (P1 & (1 << BTN_CLOSE_PIN)) != 0)
        {
            LATCH = 1;
        }

        previousP1 = P1;

        if (ledRun)
        {
            rgbPattern();
        }

        bitbangWs2812(LED_COUNT, led_data);
        mDelaymS(50);
    }
}
