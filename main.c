#include <stdint.h>
#include <stdbool.h>

#include <ch554.h>
#include <debug.h>

#include "bitbang.h"

typedef struct
{
    uint8_t  length;
    uint8_t  stations[10];
    int8_t   at;
    bool     arrives;
    uint8_t  blinkTimer;
    uint8_t *color;
    uint8_t *arriveColor;
} Subway;

// Input Pins
#define BTN_A_PIN 1  // P1.1
#define BTN_B_PIN 4  // P1.4
#define BTN_C_PIN 5  // P1.5
#define BTN_D_PIN 6  // P1.6
#define BTN_E_PIN 7  // P1.7

// Output Pins
#define LED_PIN   0  // P3.0
#define LATCH_PIN 2  // P3.2

// Define bits
SBIT(LED, 0xB0, LED_PIN);      // P3.0
SBIT(LATCH, 0xB0, LATCH_PIN);  // P3.2

// WS2812 data
#define LED_COUNT  12
#define BRIGHTNESS 15
__xdata uint8_t ledData[LED_COUNT * 3];
bool            ledChanged = false;

uint8_t red[3]    = {BRIGHTNESS, 0, 0};
uint8_t green[3]  = {0, BRIGHTNESS, 0};
uint8_t blue[3]   = {0, 0, BRIGHTNESS};
uint8_t purple[3] = {BRIGHTNESS, 0, BRIGHTNESS};
uint8_t black[3]  = {0, 0, 0};

Subway line[2] = {
    {10, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 0, false, 0, green, red},
    {5, {5, 6, 7, 8, 9}, 0, false, 0, blue, purple},
};

uint8_t gates[2] = {10, 11};

void setColor(uint8_t index, uint8_t *color)
{
    index *= 3;
    ledData[index++] = color[1];
    ledData[index++] = color[0];
    ledData[index]   = color[2];
}

void run(uint8_t i, bool forward)
{
    if (line[i].arrives)
    {
        setColor(line[i].stations[line[i].at], line[i].color);
        if (forward)
        {
            line[i].at++;
            line[i].at %= line[i].length;
        }
        else
        {
            line[i].at--;
            if (line[i].at < 0)
            {
                line[i].at += line[i].length;
            }
        }
        setColor(line[i].stations[line[i].at], line[i].arriveColor);

        ledChanged = true;
    }
    else
    {
        if (i == 0)
        {
            setColor(gates[line[i].at % 2], red);
            setColor(gates[(line[i].at + 1) % 2], green);
            ledChanged = true;
        }
    }

    line[i].arrives = !line[i].arrives;
}

void main()
{
    uint8_t previousP1 = P1;

    CfgFsys();
    mDelaymS(5);

    // Configure P3.0 and P3.4 as GPIO output
    P3_MOD_OC &= ~((1 << LATCH_PIN) & (1 << LED_PIN));
    P3_DIR_PU |= (1 << LATCH_PIN) | (1 << LED_PIN);
    LATCH = 0;  // Latch power on
    LED   = 0;

    // Configure P1.1, P1.4, P1.5, P1.6, and P1.7 as input.
    P1_MOD_OC &= ~((1 << BTN_A_PIN) & (1 << BTN_B_PIN) & (1 << BTN_C_PIN) & (1 << BTN_D_PIN) & (1 << BTN_E_PIN));
    P1_DIR_PU &= ~((1 << BTN_A_PIN) & (1 << BTN_B_PIN) & (1 << BTN_C_PIN) & (1 << BTN_D_PIN) & (1 << BTN_E_PIN));

    for (uint8_t i = 0; i < 2; i++)
    {
        setColor(line[i].stations[line[i].at], line[i].arriveColor);
    }
    setColor(gates[0], red);
    setColor(gates[1], green);

    ledChanged = true;

    while (1)
    {
        if ((previousP1 & (1 << BTN_A_PIN)) == 0 && (P1 & (1 << BTN_A_PIN)) != 0)
        {
            LATCH = 1;
        }

        if ((previousP1 & (1 << BTN_B_PIN)) == 0 && (P1 & (1 << BTN_B_PIN)) != 0)
        {
            run(0, false);
        }

        if ((previousP1 & (1 << BTN_C_PIN)) == 0 && (P1 & (1 << BTN_C_PIN)) != 0)
        {
            run(0, true);
        }

        if ((previousP1 & (1 << BTN_D_PIN)) == 0 && (P1 & (1 << BTN_D_PIN)) != 0)
        {
            run(1, false);
        }

        if ((previousP1 & (1 << BTN_E_PIN)) == 0 && (P1 & (1 << BTN_E_PIN)) != 0)
        {
            run(1, true);
        }

        previousP1 = P1;
        mDelaymS(8);  // debounce

        for (uint8_t i = 0; i < 2; i++)
        {
            if (!line[i].arrives)  // Blinks
            {
                line[i].blinkTimer++;
                if (line[i].blinkTimer == 10)  // Turn on
                {
                    setColor(line[i].stations[line[i].at], line[i].arriveColor);
                    ledChanged = true;
                }
                else if (line[i].blinkTimer == 20)  // Turn off
                {
                    line[i].blinkTimer = 0;
                    setColor(line[i].stations[line[i].at], black);
                    ledChanged = true;
                }
            }
            else
            {
                if (line[i].blinkTimer < 10)
                {
                    line[i].blinkTimer = 10;
                    setColor(line[i].stations[line[i].at], line[i].arriveColor);
                    ledChanged = true;
                }
            }
        }

        if (ledChanged)
        {
            bigBangWS2812(LED_COUNT, ledData);
            ledChanged = false;
        }
    }
}
