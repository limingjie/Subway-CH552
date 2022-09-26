#include <stdint.h>
#include <stdbool.h>
#include <ch554.h>
#include <debug.h>
#include "bitbang.h"

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
#define LED_COUNT (12)
__xdata uint8_t ledData[LED_COUNT * 3];

#define BRIGHTNESS 15

void setColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    index *= 3;
    ledData[index++] = g;
    ledData[index++] = r;
    ledData[index]   = b;
}

void main()
{
    uint8_t previousP1 = P1;
    bool    ledChanged = false;

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

    uint8_t gates[2]        = {10, 11};
    uint8_t lineStarts[2]   = {0, 5};
    uint8_t lineSize[2]     = {5, 5};
    uint8_t blinkTimer[2]   = {0, 0};
    int8_t  currentStops[2] = {0, 0};  // Line A & Line B current position.
    bool    lineRunning[2]  = {true, true};

    setColor(lineStarts[0] + currentStops[0], BRIGHTNESS, 0, 0);
    setColor(lineStarts[1] + currentStops[1], BRIGHTNESS, 0, 0);
    setColor(gates[0], BRIGHTNESS, 0, 0);
    setColor(gates[1], 0, BRIGHTNESS, 0);
    ledChanged = true;

    while (1)
    {
        if ((previousP1 & (1 << BTN_A_PIN)) == 0 && (P1 & (1 << BTN_A_PIN)) != 0)
        {
            LATCH = 1;
        }

        if ((previousP1 & (1 << BTN_B_PIN)) == 0 && (P1 & (1 << BTN_B_PIN)) != 0)
        {
            if (!lineRunning[0])
            {
                setColor(lineStarts[0] + currentStops[0]++, 0, BRIGHTNESS, 0);
                currentStops[0] %= 5;
                setColor(lineStarts[0] + currentStops[0], BRIGHTNESS, 0, 0);

                ledChanged = true;
            }
            else
            {
                setColor(gates[currentStops[0] % 2], BRIGHTNESS, 0, 0);
                setColor(gates[(currentStops[0] + 1) % 2], 0, BRIGHTNESS, 0);
            }

            lineRunning[0] = !lineRunning[0];
        }

        if ((previousP1 & (1 << BTN_C_PIN)) == 0 && (P1 & (1 << BTN_C_PIN)) != 0)
        {
            if (!lineRunning[0])
            {
                setColor(lineStarts[0] + currentStops[0]--, 0, BRIGHTNESS, 0);
                if (currentStops[0] < 0)
                {
                    currentStops[0] += 5;
                }
                setColor(lineStarts[0] + currentStops[0], BRIGHTNESS, 0, 0);

                ledChanged = true;
            }
            else
            {
                setColor(gates[currentStops[0] % 2], BRIGHTNESS, 0, 0);
                setColor(gates[(currentStops[0] + 1) % 2], 0, BRIGHTNESS, 0);
            }

            lineRunning[0] = !lineRunning[0];
        }

        if ((previousP1 & (1 << BTN_D_PIN)) == 0 && (P1 & (1 << BTN_D_PIN)) != 0)
        {
            if (!lineRunning[1])
            {
                setColor(lineStarts[1] + currentStops[1]++, 0, BRIGHTNESS, 0);
                currentStops[1] %= 5;
                setColor(lineStarts[1] + currentStops[1], BRIGHTNESS, 0, 0);

                ledChanged = true;
            }

            lineRunning[1] = !lineRunning[1];
        }

        if ((previousP1 & (1 << BTN_E_PIN)) == 0 && (P1 & (1 << BTN_E_PIN)) != 0)
        {
            if (!lineRunning[1])
            {
                setColor(lineStarts[1] + currentStops[1]--, 0, BRIGHTNESS, 0);
                if (currentStops[1] < 0)
                {
                    currentStops[1] += 5;
                }
                setColor(lineStarts[1] + currentStops[1], BRIGHTNESS, 0, 0);

                ledChanged = true;
            }

            lineRunning[1] = !lineRunning[1];
        }

        previousP1 = P1;
        mDelaymS(8);  // debounce

        for (uint8_t i = 0; i < 2; i++)
        {
            if (lineRunning[i])  // Blinks
            {
                blinkTimer[i]++;
                if (blinkTimer[i] == 10)  // Turn on
                {
                    setColor(lineStarts[i] + currentStops[i], BRIGHTNESS, 0, 0);
                    ledChanged = true;
                }
                else if (blinkTimer[i] == 20)  // Turn off
                {
                    blinkTimer[i] = 0;
                    setColor(lineStarts[i] + currentStops[i], 0, 0, 0);
                    ledChanged = true;
                }
            }
            else
            {
                if (blinkTimer[i] < 10)
                {
                    blinkTimer[i] = 10;
                    setColor(lineStarts[i] + currentStops[i], BRIGHTNESS, 0, 0);
                    ledChanged = true;
                }
            }
        }

        if (ledChanged)
        {
            bitbangWs2812(LED_COUNT, ledData);
            ledChanged = false;
        }
    }
}
