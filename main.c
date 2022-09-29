#include <stdint.h>

#include <ch554.h>
#include <debug.h>

#include "bitbang.h"

// Input Pins
#define KEY_A_PIN 1  // P1.1
#define KEY_B_PIN 4  // P1.4
#define KEY_C_PIN 5  // P1.5
#define KEY_D_PIN 6  // P1.6
#define KEY_E_PIN 7  // P1.7

// All key pins are pulled up.
// - A key pressed if input bit changes from 1 to 0.
// - A key released if input bit changes from 0 to 1.
#define KEY_PRESSED(KEY)  ((previousP1 & (1 << (KEY))) && !(P1 & (1 << (KEY))))
#define KEY_RELEASED(KEY) (!(previousP1 & (1 << (KEY))) && (P1 & (1 << (KEY))))
#define DEBOUNCE_INTERVAL 8

// Output Pins
#define LED_PIN   0  // P3.0 - WS2812 LED Strip
#define LATCH_PIN 2  // P3.2 - Power latch

// Define bits
SBIT(LED, 0xB0, LED_PIN);      // P3.0
SBIT(LATCH, 0xB0, LATCH_PIN);  // P3.2

// WS2812
#define LED_COUNT  27
#define BRIGHTNESS 15
// External data memory
__xdata uint8_t      ledData[LED_COUNT * 3];
__bit                ledChanged = 0;
__data const uint8_t RED[3]     = {BRIGHTNESS, 0, 0};
__data const uint8_t GREEN[3]   = {0, BRIGHTNESS, 0};
__data const uint8_t BLUE[3]    = {0, 0, BRIGHTNESS};
__data const uint8_t PURPLE[3]  = {BRIGHTNESS, 0, BRIGHTNESS};
__data const uint8_t BLACK[3]   = {0, 0, 0};

#define SUBWAY_COLOR         0
#define SUBWAY_ARRIVAL_COLOR 1

typedef struct
{
    const uint8_t         stops[25];
    const uint8_t         length;
    uint8_t               at;
    uint8_t               arrives;
    uint8_t               blinkTimer;
    __data const uint8_t *colors[2];  // `__data` -> 8-bit pointer
} Subway;

__data Subway line[2] = {
    {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, 10, 0, 0, 0, {GREEN, RED}},
    {{5, 6, 7, 8, 9}, 5, 0, 0, 0, {BLUE, PURPLE}},
};

uint8_t gates[2] = {10, 11};

uint16_t idleTimer = 0;

void setColor(uint8_t index, __data const uint8_t *color)  // `__data` -> 8-bit pointer
{
    index *= 3;
    ledData[index++] = color[1];
    ledData[index++] = color[0];
    ledData[index]   = color[2];
}

void initSubway()
{
    setColor(gates[0], RED);
    setColor(gates[1], GREEN);

    ledChanged = 1;
}

void runSubway(uint8_t i, __bit forward)
{
    __data Subway *l = &line[i];  // `__data` -> 8-bit pointer
    if (l->arrives)
    {
        setColor(l->stops[l->at], l->colors[SUBWAY_COLOR]);
        if (forward)
        {
            if (++l->at >= l->length)
            {
                l->at = 0;
            }
        }
        else
        {
            if (l->at == 0)
            {
                l->at = l->length;
            }
            l->at--;
        }
        setColor(l->stops[l->at], l->colors[SUBWAY_ARRIVAL_COLOR]);

        ledChanged = 1;
    }
    else
    {
        if (i == 0)
        {
            setColor(gates[l->at & 0x01], RED);
            setColor(gates[(l->at + 1) & 0x01], GREEN);
            ledChanged = 1;
        }
    }

    l->arrives = !l->arrives;

    idleTimer = 0;  // Reset idleTimer
}

void blinkLights()
{
    for (uint8_t i = 0; i < 2; i++)
    {
        __data Subway *l = &line[i];  // `__data` -> 8-bit pointer
        if (!l->arrives)              // Blinks
        {
            l->blinkTimer++;
            if (l->blinkTimer == 10)  // Turn on
            {
                setColor(l->stops[l->at], l->colors[SUBWAY_ARRIVAL_COLOR]);
                ledChanged = 1;
            }
            else if (l->blinkTimer == 20)  // Turn off
            {
                l->blinkTimer = 0;
                setColor(l->stops[l->at], BLACK);
                ledChanged = 1;
            }
        }
        else  // Reset from blinking
        {
            if (l->blinkTimer < 10)
            {
                l->blinkTimer = 10;
                setColor(l->stops[l->at], l->colors[SUBWAY_ARRIVAL_COLOR]);
                ledChanged = 1;
            }
        }
    }
}

void processEvents()
{
    // All key pins are pulled up.
    static uint8_t previousP1 = 0xFF;

    if (KEY_RELEASED(KEY_A_PIN))
    {
        LATCH = 1;  // Power off
    }
    if (KEY_RELEASED(KEY_B_PIN))
    {
        runSubway(0, 0);
    }
    if (KEY_RELEASED(KEY_C_PIN))
    {
        runSubway(0, 1);
    }
    if (KEY_RELEASED(KEY_D_PIN))
    {
        runSubway(1, 0);
    }
    if (KEY_RELEASED(KEY_E_PIN))
    {
        runSubway(1, 1);
    }

    // Debounce
    previousP1 = P1;
    mDelaymS(DEBOUNCE_INTERVAL);
}

void init()
{
    CfgFsys();
    mDelaymS(5);

    // Configure P3.0 and P3.4 as GPIO output
    P3_MOD_OC &= ~((1 << LATCH_PIN) & (1 << LED_PIN));
    P3_DIR_PU |= (1 << LATCH_PIN) | (1 << LED_PIN);
    LATCH = 0;  // Latch power on
    LED   = 0;

    // Configure P1.1, P1.4, P1.5, P1.6, and P1.7 as input.
    P1_MOD_OC &= ~((1 << KEY_A_PIN) & (1 << KEY_B_PIN) & (1 << KEY_C_PIN) & (1 << KEY_D_PIN) & (1 << KEY_E_PIN));
    P1_DIR_PU &= ~((1 << KEY_A_PIN) & (1 << KEY_B_PIN) & (1 << KEY_C_PIN) & (1 << KEY_D_PIN) & (1 << KEY_E_PIN));
}

void main()
{
    init();

    initSubway();

    while (1)
    {
        // Each loop takes around 10 milliseconds.
        // 6,000 loops is around 60 seconds.
        if (idleTimer++ >= 6000)
        {
            LATCH = 1;  // Power off
        }

        processEvents();

        blinkLights();

        if (ledChanged)
        {
            bigBangWS2812(LED_COUNT, ledData);
            ledChanged = 0;
        }
    }
}
