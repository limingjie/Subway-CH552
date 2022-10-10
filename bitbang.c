#include <stdint.h>
#include <ch554.h>
#include "bitbang.h"

void bigBangWS2812(uint8_t ledCount, __xdata uint8_t* ledData)
{
    ledCount;
    ledData;

    // Bitbang routine
    // Input parameters: (determined by compilation)
    // * byteCount should be allocated in dpl
    // * ledData should be allocated with name '_bigBangWS2812_PARAM_2'

    // Strategy:
    // * Keep the data memory pointer in DPTR
    // * Keep ledCount in r2
    // * Store bitCount in r3
    // * Store the current data variable in ACC

    //  TODO: What context needs to be pushed/popped?

    __asm__(
        "mov r2, dpl             ; Load the LED count into r2                                \n"

        "mov dpl, _bigBangWS2812_PARM_2  ; Load the LED data start address into DPTR         \n"
        "mov dph, (_bigBangWS2812_PARM_2 + 1)                                                \n"

        "00001$:                 ; byte loop                                                 \n"

        // Red byte
        "    movx a,@dptr        ; Load the current LED data value into the accumulator (1)  \n"
        "    inc dptr            ; and advance the counter for the next LED data value (1)   \n"

        "    mov r3, #8          ; Set up the bit loop (2)                                   \n"
        "00002$:                 ; red bit loop                                              \n"
        "    setb _LED           ; Begin bit cycle- set bit high (2)                         \n"

        "    nop                 ; Tune this count by hand, want ~.4uS (1*2)                 \n"
        "    nop                                                                             \n"

        "    rlc A               ; Shift the LED data value left to get the high bit (1)     \n"
        "    mov _LED, C         ; Set the output bit high if the current bit is high, (2)   \n"
        "                        ; otherwise set it low                                      \n"

        "    nop                 ; Tune this count by hand, want ~.4uS (1*2)                 \n"
        "    nop                                                                             \n"

        "    clr _LED            ; final part of bit cycle, set bit low (2)                  \n"

        // "    nop                 ; Tune this count by hand, want ~.45uS                      \n"
        // "    nop                                                                             \n"

        "    djnz r3, 00002$     ; If there are more bits in this byte (2, ?)                \n"

        // Green byte
        "    movx a,@dptr        ; Load the current LED data value into the accumulator (1)  \n"
        "    inc dptr            ; and advance the counter for the next LED data value (1)   \n"

        "    mov r3, #8          ; Set up the bit loop (2)                                   \n"
        "00003$:                 ; red bit loop                                              \n"
        "    setb _LED           ; Begin bit cycle- set bit high (2)                         \n"

        "    nop                 ; Tune this count by hand, want ~.4uS (1*2)                 \n"
        "    nop                                                                             \n"

        "    rlc A               ; Shift the LED data value left to get the high bit (1)     \n"
        "    mov _LED, C         ; Set the output bit high if the current bit is high, (2)   \n"
        "                        ; otherwise set it low                                      \n"

        "    nop                 ; Tune this count by hand, want ~.4uS (1*2)                 \n"
        "    nop                                                                             \n"

        "    clr _LED            ; final part of bit cycle, set bit low (2)                  \n"

        // "    nop                 ; Tune this count by hand, want ~.45uS                      \n"
        // "    nop                                                                             \n"

        "    djnz r3, 00003$     ; If there are more bits in this byte (2, ?)                \n"

        // Blue byte
        "    movx a,@dptr        ; Load the current LED data value into the accumulator (1)  \n"
        "    inc dptr            ; and advance the counter for the next LED data value (1)   \n"

        "    mov r3, #8          ; Set up the bit loop (2)                                   \n"
        "00004$:                 ; red bit loop                                              \n"
        "    setb _LED           ; Begin bit cycle- set bit high (2)                         \n"

        "    nop                 ; Tune this count by hand, want ~.4uS (1*2)                 \n"
        "    nop                                                                             \n"

        "    rlc A               ; Shift the LED data value left to get the high bit (1)     \n"
        "    mov _LED, C         ; Set the output bit high if the current bit is high, (2)   \n"
        "                        ; otherwise set it low                                      \n"

        "    nop                 ; Tune this count by hand, want ~.4uS (1*2)                 \n"
        "    nop                                                                             \n"

        "    clr _LED            ; final part of bit cycle, set bit low (2)                  \n"

        // "    nop                 ; Tune this count by hand, want ~.45uS                      \n"
        // "    nop                                                                             \n"
        "    djnz r3, 00004$     ; If there are more bits in this byte (2, ?)                \n"

        "    djnz r2, 00001$     ; If there are more LEDs (2, ?)                             \n");
}
