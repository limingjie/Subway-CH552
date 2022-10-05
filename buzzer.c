#include "buzzer.h"

#include <debug.h>

#define BUZZER_PIN 4             // P3.4 - Buzzer
SBIT(BUZZER, 0xB0, BUZZER_PIN);  // P3.4

// Half period of the notes
// -> 1,000,000µs / Note_Frequency / 2
__xdata const uint16_t HALF_NOTE_IN_uS[] = {
    1911, 1703, 1517, 1432, 1276, 1136, 1012,  // C4 - B4
    956,  851,  758,  716,  638,  568,  506    // C5 - B5
};

// Number of notes every 100 ms
// -> Note_Frequency / 10
__xdata const uint8_t NUM_NOTES_100mS[] = {
    26, 29, 33, 35, 39, 44, 49,  // C4 - B4
    52, 59, 66, 70, 78, 88, 99   // C5 - B5
};

void initBuzzer()
{
    P3_MOD_OC &= ~(1 << BUZZER_PIN);
    P3_DIR_PU |= 1 << BUZZER_PIN;
    BUZZER = 0;
}

void playBuzzer(__xdata const uint8_t *melody, const uint8_t melody_length)
{
    for (uint8_t note = 0; note < melody_length; note++)
    {
        // Get a note
        uint8_t  loop  = NUM_NOTES_100mS[*melody];
        uint16_t delay = HALF_NOTE_IN_uS[*melody++];
        // Play the note for length x 100ms.
        while (loop--)
        {
            uint8_t length = *melody;
            while (length--)
            {
                BUZZER = 1;
                mDelayuS(delay);
                BUZZER = 0;
                mDelayuS(delay);
            }
        }

        melody++;

        // Pause a bit
        mDelaymS(50);
    }
}
