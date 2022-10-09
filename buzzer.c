#include "buzzer.h"

#include <debug.h>

// Schematic
//                             VCC
//                              |
//                        ______|
//           1n4001      _|_    |
//       Flyback Diode   /_\  [Passive Buzzer]
//                        |_____|
//                              |
//                180Ω       b  | c
//  GPIO <-------[/\/\]-------|<     SS8050
//                              | e   NPN
//                              |
//                              |
//                             GND

#define BUZZER_PIN 4             // P3.4 - Buzzer
SBIT(BUZZER, 0xB0, BUZZER_PIN);  // P3.4

// The length of half period of notes in µs.
// -> 1,000,000µs / Note_Frequency / 2
__xdata const uint16_t NOTES_HALF_PERIOD_us[] = {
    1911, 1703, 1517, 1432, 1276, 1136, 1012,  // C4 - B4
    956,  851,  758,  716,  638,  568,  506    // C5 - B5
};

// Number of waves every 100 ms.
// -> Note_Frequency / 10
__xdata const uint8_t NOTES_WAVES_IN_100ms[] = {
    26, 29, 33, 35, 39, 44, 49,  // C4 - B4
    52, 59, 66, 70, 78, 88, 99   // C5 - B5
};

void initBuzzer()
{
    P3_MOD_OC &= ~(1 << BUZZER_PIN);
    P3_DIR_PU |= 1 << BUZZER_PIN;
    BUZZER = 0;
}

// Play a melody
// - melody = [melody length, note_0, note_0_beats, note_1, note_1_beats...]
//   - a beat last 100ms.
//   - a 50ms pause is placed between 2 notes.
void playBuzzer(__xdata const uint8_t *melody)
{
    const uint8_t length = *melody++;
    for (uint8_t note = 0; note < length; note++)
    {
        uint8_t  waves = NOTES_WAVES_IN_100ms[*melody];    // The number of waves in 100ms.
        uint16_t delay = NOTES_HALF_PERIOD_us[*melody++];  // The half period of the wave.
        // The 2 while loops play the note waves x beats times, which last 100ms x beats.
        while (waves--)
        {
            uint8_t beats = *melody;
            while (beats--)
            {
                BUZZER = 1;
                mDelayuS(delay);
                BUZZER = 0;
                mDelayuS(delay);
            }
        }

        melody++;

        // Pause between 2 notes
        mDelaymS(50);
    }
}
