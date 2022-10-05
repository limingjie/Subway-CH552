#include <stdint.h>

#include <ch554.h>
#include <debug.h>

#include "bitbang.h"
#include "buzzer.h"

// Input Pins
#define KEY_A_PIN 1  // P1.1
#define KEY_B_PIN 7  // P1.7
#define KEY_C_PIN 6  // P1.6
#define KEY_D_PIN 5  // P1.5
#define KEY_E_PIN 4  // P1.4

// All key pins are pulled up.
// - A key pressed if input bit changes from 1 to 0.
// - A key released if input bit changes from 0 to 1.
#define KEY_PRESSED(KEY)  ((lastP1 & (1 << (KEY))) && !(P1 & (1 << (KEY))))
#define KEY_RELEASED(KEY) (!(lastP1 & (1 << (KEY))) && (P1 & (1 << (KEY))))
#define DEBOUNCE_INTERVAL 8  // 8 ms

// Output Pins
#define LED_PIN  0  // P3.0 - WS2812 LED Strip
#define KILL_PIN 3  // P3.3 - Power control

// Define bits
SBIT(LED, 0xB0, LED_PIN);    // P3.0
SBIT(KILL, 0xB0, KILL_PIN);  // P3.3

// WS2812
#define LED_COUNT  27
#define BRIGHTNESS 15
__xdata uint8_t      ledData[LED_COUNT * 3];  // In external data memory
__bit                ledChanged = 0;
__data const uint8_t RED[3]     = {BRIGHTNESS, 0, 0};
__data const uint8_t GREEN[3]   = {0, BRIGHTNESS, 0};
__data const uint8_t BLUE[3]    = {0, 0, BRIGHTNESS};
__data const uint8_t PURPLE[3]  = {BRIGHTNESS, 0, BRIGHTNESS};
__data const uint8_t BLACK[3]   = {0, 0, 0};

typedef struct
{
    const uint8_t               stops[20];
    const uint8_t               length;
    uint8_t                     at;
    uint8_t                     running;       // __bit will be promoted to int here
    __data const uint8_t *const color;         // `__data` -> 8-bit pointer
    __data const uint8_t *const arrivalColor;  // `__data` -> 8-bit pointer
} Subway;

__data Subway line[2] = {
    {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},  // .stops
        10,                              // .length
        0,                               // .at
        1,                               // .running
        GREEN,                           // .color
        RED,                             // .arrivalColor
    },
    {
        {5, 6, 7, 8, 9},  // .stops
        5,                // .length
        0,                // .at
        1,                // .running
        BLUE,             // .color
        PURPLE,           // .arrivalColor
    },
};

__data const uint8_t gates[2] = {10, 11};

uint16_t idleTimer = 0;

void setColor(uint8_t index, __data const uint8_t *color)  // `__data` -> 8-bit pointer
{
    index *= 3;
    ledData[index]   = color[1];
    ledData[++index] = color[0];
    ledData[++index] = color[2];
}

void initSubway()
{
    setColor(gates[0], RED);
    setColor(gates[1], GREEN);

    ledChanged = 1;
}

void runSubway(uint8_t i, __bit forward)
{
    __data Subway *const l           = &line[i];  // `__data` -> 8-bit pointer
    __data const uint8_t currentStop = l->stops[l->at];

    l->running = !l->running;

    // 1. Arrived -> Running (running == true)
    //    - Set current stop to `color`
    //    - Move to next stop
    // 2. Running -> Arrived (running == false)
    //    - Set current stop to `arrivalColor`
    if (l->running)
    {
        const uint8_t length = l->length;

        setColor(currentStop, l->color);
        if (forward)
        {
            if (++l->at == length)
            {
                l->at = 0;
            }
        }
        else
        {
            if (l->at == 0)
            {
                l->at = length;
            }
            l->at--;
        }
    }
    else
    {
        setColor(currentStop, l->arrivalColor);

        if (i == 0)
        {
            uint8_t at = l->at;
            setColor(gates[at & 0x01], GREEN);
            setColor(gates[++at & 0x01], RED);
        }
    }

    ledChanged = 1;

    idleTimer = 0;  // Reset idleTimer
}

void blinkLights()
{
    static uint8_t blinkTimer;

    for (uint8_t i = 0; i < 2; ++i)
    {
        __data Subway *l = &line[i];  // `__data` -> 8-bit pointer
        if (l->running)               // Blinks
        {
            if (blinkTimer == 0)  // Turn on
            {
                setColor(l->stops[l->at], l->arrivalColor);
                ledChanged = 1;
            }
            else if (blinkTimer == 10)  // Turn off
            {
                setColor(l->stops[l->at], BLACK);
                ledChanged = 1;
            }
        }
    }

    if (++blinkTimer >= 20)
    {
        blinkTimer = 0;
    }
}

void processEvents()
{
    // All key pins are pulled up.
    static uint8_t lastP1 = 0xFF;

    if (KEY_RELEASED(KEY_A_PIN))
    {
        KILL = 1;  // Power off
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
    lastP1 = P1;
    mDelaymS(DEBOUNCE_INTERVAL);
}

void init()
{
    CfgFsys();
    mDelaymS(5);

    // Configure P3.0 and P3.3 in push-pull output mode.
    P3_MOD_OC &= ~((1 << KILL_PIN) | (1 << LED_PIN));
    P3_DIR_PU |= (1 << KILL_PIN) | (1 << LED_PIN);
    KILL = 0;  // Power on

    // Configure P1.1, P1.4, P1.5, P1.6, and P1.7 in Quasi-Bidirectional mode, support input with internal pull-up.
    P1_MOD_OC &= (1 << KEY_A_PIN) | (1 << KEY_B_PIN) | (1 << KEY_C_PIN) | (1 << KEY_D_PIN) | (1 << KEY_E_PIN);
    P1_DIR_PU &= (1 << KEY_A_PIN) | (1 << KEY_B_PIN) | (1 << KEY_C_PIN) | (1 << KEY_D_PIN) | (1 << KEY_E_PIN);

    // Turn off LDO
    GLOBAL_CFG |= bLDO3V3_OFF;

    // Simplified code from CH554 ADC example.
    // - Enable ADC
    ADC_CFG &= ~bADC_CLK | 0;  // Fast mode, 96 clock cycles.
    ADC_CFG |= bADC_EN;        // ADC power enable
    // - Enable ADC channel 3, P3.2
    ADC_CHAN1 = 1;
    ADC_CHAN0 = 1;
    P3_DIR_PU &= ~bAIN3;
}

void batteryCheck()
{
    static uint16_t batteryMonitorTimer = 0;

    // Check battery voltage on the first second of every checking cycle.
    if (batteryMonitorTimer == 100)
    {
        ADC_START = 1;     // Start ADC sampling.
        while (ADC_START)  // Sampling completes when ADC_START becomes 0.
        {
        }

        // Stop if the battery voltage drops under 3.0V.
        // The ADC pin is connect using a 2x100k voltage divider, thus monitoring 1.5V.
        // Assume the voltage supply is 3.3V: 3.3V * 116 / 255 = 1.50V
        if (ADC_DATA < 116)  // Low voltage detected
        {
            KILL = 1;
        }
    }

    // Set checking cycle, reset timer every around 10 seconds
    if (++batteryMonitorTimer == 1000)
    {
        batteryMonitorTimer = 0;
    }
}

void main()
{
    init();

    initBuzzer();
    buzzer();

    initSubway();

    while (1)
    {
        // Power off after 1 min idle time.
        // Each loop takes around 10 milliseconds, 6,000 loops is around 60 seconds.
        // Use 8-bit instructions to save a few bytes for the binary.
        // 6,000 >> 8 = 23 and 23 << 8 = 5,888, close enough.
        if ((uint8_t)((++idleTimer) >> 8) == 23)
        {
            KILL = 1;  // Power off
        }

        batteryCheck();

        processEvents();

        blinkLights();

        if (ledChanged)
        {
            bigBangWS2812(LED_COUNT, ledData);
            ledChanged = 0;
        }
    }
}
