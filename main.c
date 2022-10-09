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
#define KEY_PRESSED(KEY)  ((lastKeyState & (1 << (KEY))) && !(P1 & (1 << (KEY))))
#define KEY_RELEASED(KEY) (!(lastKeyState & (1 << (KEY))) && (P1 & (1 << (KEY))))
#define DEBOUNCE_INTERVAL 8  // 8 ms

// Output Pins
#define LED_PIN      0  // P3.0 - WS2812 LED Strip
#define SHUTDOWN_PIN 3  // P3.3 - Power control

// Define bits
SBIT(LED, 0xB0, LED_PIN);            // P3.0
SBIT(SHUTDOWN, 0xB0, SHUTDOWN_PIN);  // P3.3

// All the time variables are in milliseconds
__data uint32_t systemTime     = 0;
__data uint32_t lastActionTime = 0;
// __data uint32_t previousTime   = 0;

// Menu
// 0 - Subway
// 1 - Blink LEDs
__data uint8_t menuIndex = 0;

// WS2812
#define LED_COUNT  27
#define BRIGHTNESS 5
__xdata uint8_t      ledData[LED_COUNT * 3];  // In external data memory
__bit                ledDataChanged = 0;
__data const uint8_t RED[3]         = {BRIGHTNESS, 0, 0};
__data const uint8_t GREEN[3]       = {0, BRIGHTNESS, 0};
__data const uint8_t BLUE[3]        = {0, 0, BRIGHTNESS};
__data const uint8_t YELLOW[3]      = {BRIGHTNESS, BRIGHTNESS, 0};
__data const uint8_t PURPLE[3]      = {BRIGHTNESS, 0, BRIGHTNESS};
__data const uint8_t BLACK[3]       = {0, 0, 0};
__data const uint8_t BRIGHT[3]      = {255, 255, 255};

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
        {
            19, 0,  1,  2,  3,   // .stops
            4,  5,  6,  7,  8,   //
            9,  10, 11, 12, 13,  //
            14, 15, 16, 17, 18   //
        },
        20,     // .length
        0,      // .at
        1,      // .running
        GREEN,  // .color
        RED,    // .arrivalColor
    },
    {
        {
            11, 12, 13, 14, 15,  // .stops
            16, 17, 18, 19, 20,  //
            21, 22, 23, 24       //
        },
        14,      // .length
        0,       // .at
        1,       // .running
        BLUE,    // .color
        PURPLE,  // .arrivalColor
    },
};

__data const uint8_t subwayGates[2] = {25, 26};

__xdata const uint8_t theStarSong[] = {
    14,                                               // length
    C4, 2, C4, 2, G4, 2, G4, 2, A4, 2, A4, 2, G4, 4,  // 1 1 | 5 5 | 6 6 | 5 -
    F4, 2, F4, 2, E4, 2, E4, 2, D4, 2, D4, 2, C4, 4   // 4 4 | 3 3 | 2 2 | 1 -
};
__xdata const uint8_t startupSound[]  = {5, E5, 2, B4, 2, A4, 3, E5, 2, B4, 4};
__xdata const uint8_t shutdownSound[] = {4, A5, 1, E5, 1, A4, 1, B4, 2};
__xdata const uint8_t warningSound[]  = {3, C4, 2, C4, 2, C4, 2};

void setColor(uint8_t index, __data const uint8_t *color)  // `__data` -> 8-bit pointer
{
    index *= 3;
    ledData[index]   = color[1];
    ledData[++index] = color[0];
    ledData[++index] = color[2];
}

void initSubway()
{
    for (uint8_t i = 0; i < LED_COUNT; i++)
    {
        setColor(i, YELLOW);
    }

    ledDataChanged = 1;
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
            setColor(subwayGates[at & 0x01], YELLOW);
            setColor(subwayGates[++at & 0x01], BLUE);
        }
    }

    ledDataChanged = 1;
    lastActionTime = systemTime;
}

void blinkSubwayLights()
{
    __xdata static uint32_t lastBlinkTime = 0;
    static __bit            on            = 0;

    if (systemTime - lastBlinkTime >= (on ? 700 : 300))
    {
        for (uint8_t i = 0; i < 2; ++i)
        {
            __data Subway *l = &line[i];  // `__data` -> 8-bit pointer
            if (l->running)
            {
                setColor(l->stops[l->at], on ? BLACK : l->arrivalColor);
            }
        }

        on             = !on;
        ledDataChanged = 1;
        lastBlinkTime  = systemTime;
    }
}

void shutdown()
{
    playBuzzer(shutdownSound);
    SHUTDOWN = 1;
}

void timer0_interrupt(void) __interrupt(INT_NO_TMR0)
{
    // (0x10000 - 0xFC18) x (1 / (12000000 / 12))) = 0.001s
    TH0 = 0xFC;
    TL0 = 0x18;

    systemTime++;
}

void blinkLights()
{
    __xdata static uint32_t lastBlinkTime = 0;
    static uint8_t          blinkCounter  = 0;

    // Blink every 500ms
    if (systemTime - lastBlinkTime >= 500)
    {
        __bit on = blinkCounter++ & 0x01;
        for (uint8_t i = 0; i < LED_COUNT; i++)
        {
            setColor(i, on ? YELLOW : BLACK);
            on = !on;
        }

        ledDataChanged = 1;
        lastBlinkTime  = systemTime;
    }
}

void processEvents()
{
    __xdata static uint32_t lastDebounceTime = 0;
    static uint8_t          lastKeyState     = 0xFF;  // All key pins are pulled up.

    if (systemTime - lastDebounceTime >= DEBOUNCE_INTERVAL)
    {
        if (KEY_RELEASED(KEY_A_PIN))
        {
            if (menuIndex == 0)
                runSubway(0, 0);
        }
        if (KEY_RELEASED(KEY_B_PIN))
        {
            if (menuIndex == 0)
                runSubway(0, 1);
        }
        if (KEY_RELEASED(KEY_C_PIN))
        {
            if (menuIndex == 0)
                runSubway(1, 0);
        }
        if (KEY_RELEASED(KEY_D_PIN))
        {
            if (menuIndex == 0)
                runSubway(1, 1);
        }
        if (KEY_RELEASED(KEY_E_PIN))
        {
            if (++menuIndex == 2)
            {
                menuIndex = 0;
            }

            // Reset subway lights
            if (menuIndex == 0)
            {
                initSubway();
            }
            // playBuzzer(theStarSong);
        }

        lastKeyState     = P1;
        lastDebounceTime = systemTime;
    }
}

void startup()
{
    CfgFsys();
    mDelaymS(5);

    // Configure P3.0 and P3.3 in push-pull output mode.
    // - Pn_MOD_OC = 0 (Push-Pull)
    // - Pn_DIR_PU = 1 (Output)
    P3_MOD_OC &= ~((1 << SHUTDOWN_PIN) | (1 << LED_PIN));
    P3_DIR_PU |= (1 << SHUTDOWN_PIN) | (1 << LED_PIN);
    SHUTDOWN = 0;  // Power on

    // Configure P1.1, P1.4, P1.5, P1.6, and P1.7 in Quasi-Bidirectional mode, support input with internal pull-up.
    // - Pn_MOD_OC = 1 (Open-Drain)
    // - Pn_DIR_PU = 1 (Pull-Up)
    P1_MOD_OC |= (1 << KEY_A_PIN) | (1 << KEY_B_PIN) | (1 << KEY_C_PIN) | (1 << KEY_D_PIN) | (1 << KEY_E_PIN);
    P1_DIR_PU |= (1 << KEY_A_PIN) | (1 << KEY_B_PIN) | (1 << KEY_C_PIN) | (1 << KEY_D_PIN) | (1 << KEY_E_PIN);

    // Turn off LDO
    GLOBAL_CFG |= bLDO3V3_OFF;

    // Simplified code from CH554 ADC example.
    // - Enable ADC
    ADC_CFG |= bADC_CLK;  // ADC_CLK (bit 0): 0 -> slow mode, 384 Fosc cycles for each ADC
                          //                  1 -> fast mode,  96 Fosc cycles for each ADC
    ADC_CFG |= bADC_EN;   // ADC_EN  (bit 3): Power enable bit of ADC module
    // - Enable ADC channel 3, P3.2
    ADC_CHAN0 = 1;
    ADC_CHAN1 = 1;
    P3_DIR_PU &= ~bAIN3;

    // Enable interrupt globally
    EA = 1;
    // Enable timer0 interrupt
    ET0 = 1;
    // Set timer0 to Mode 1 - 16-bit counter
    // - The reset value of TMOD = 0x00
    // - Set bT0_M1 = 0 and bT0_M0 = 1
    TMOD |= bT0_M0;
    // Start timer 0
    TR0 = 1;

    // Play startup sound
    initBuzzer();
    playBuzzer(startupSound);
}

void batteryCheck()
{
    static uint16_t lastCheckTime   = 0;
    static uint8_t  batteryLowCount = 0;

    // Check battery voltage every 10 seconds
    if (systemTime - lastCheckTime >= 10000)
    {
        ADC_START = 1;     // Start ADC sampling.
        while (ADC_START)  // Sampling completes when ADC_START becomes 0.
        {
        }

        // Shutdown if the battery voltage detected under 3.0V for 3 times.
        // The ADC pin is connect using a 2x100k voltage divider, thus monitoring 1.5V.
        // Assume the voltage supply is 3.3V: 255 x 1.5V / 3.3V = 115.9
        if (ADC_DATA < 116)  // Low voltage detected
        {
            playBuzzer(warningSound);
            if (++batteryLowCount >= 3)
            {
                shutdown();
            }
        }
        else
        {
            batteryLowCount = 0;
        }

        lastCheckTime = systemTime;
    }
}

void main()
{
    startup();
    initSubway();

    while (1)
    {
        // Power off after idle for 5 minutes.
        if (systemTime - lastActionTime >= 300000)
        {
            shutdown();
        }

        batteryCheck();

        processEvents();

        switch (menuIndex)
        {
            case 0:
                blinkSubwayLights();
                break;
            case 1:
                blinkLights();
                break;
        }

        if (ledDataChanged)
        {
            EA = 0;  // Disable interrupt globally, to avoid interrupting WS2812 data transmission.
            bigBangWS2812(LED_COUNT, ledData);
            EA = 1;  // Re-enable interrupt globally

            ledDataChanged = 0;
        }
    }
}
