/* -*- Mode: C++; c-basic-offset: 4 -*-
 *
 * Sound engine for Robot Odyssey DS. This is a special-purpose PC
 * speaker emulator which runs mainly on the ARM7. It is designed to
 * handle the PC speaker mode Robot Odyssey uses, in which the speaker
 * output is used as a 1-bit DAC.
 *
 * Copyright (c) 2009 Micah Dowty <micah@navi.cx>
 *
 *    Permission is hereby granted, free of charge, to any person
 *    obtaining a copy of this software and associated documentation
 *    files (the "Software"), to deal in the Software without
 *    restriction, including without limitation the rights to use,
 *    copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following
 *    conditions:
 *
 *    The above copyright notice and this permission notice shall be
 *    included in all copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *    OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _SOUNDENGINE_H_
#define _SOUNDENGINE_H_

#include <nds.h>

class SoundEngine
{
 public:
    static void init(int timerChannel = 0);

    static void writeSpeakerTimestamp(uint32_t timestamp) {
        while (1) {
            uint16_t head = getIPC()->head;
            uint16_t tail = getIPC()->tail;
            uint16_t nextHead = (head + 1) & (FIFO_SIZE - 1);

            if (nextHead == tail) {
                /* FIFO is about to overflow. Busy-wait until the ARM7 drains it some. */
                continue;
            }

            getIPC()->fifo[head] = timestamp;
            getIPC()->head = nextHead;

            break;
        };
    }

 private:
    /*
     * Buffer size MUST be a power of two. It can't be larger than 2kB
     * currently. You may need to adjust getIPC() if you change this.
     */
    static const uint16_t FIFO_SIZE = 512;

    /*
     * FIFO buffer for holding PC speaker data, in the form of cycle
     * timestamps for edge transitions. This edge data is generated by
     * the I/O emulator running on the ARM9, and consumed by the sound
     * engine on the ARM7.
     */
    struct IPCBuffer
    {
        uint16_t head;             // First available FIFO slot for writer
        uint16_t tail;             // First full FIFO slot for reader
        uint32_t fifo[FIFO_SIZE];  // 2kB of buffer space for edge timestamps
    };

    /*
     * Locate the sound IPC region in the last page of main memory,
     * just before libnds's TransferRegion and PersonalData. Use the
     * noncached mirror of main RAM, so we don't have to worry about
     * flushing the ARM9's cache when we write to it.
     *
     * XXX: This is kind of a hack, but libnds isn't really set up
     *      to let us allocate this with the linker's help.
     */
    static inline volatile IPCBuffer* getIPC() {
        return (volatile IPCBuffer*) 0x027FE000;
    }

    /*
     * Sound channel usage
     */

    static const int CHANNEL_PCSPEAKER = 0;

    /*
     * Audio parameters
     */

    static const int PC_CLOCK_HZ = 4770000;  // 4.77 MHz
    static const int SAMPLE_RATE = 16384;
    static const int SAMPLES_PER_FRAME = 256;
    static const int BUFFER_SIZE = SAMPLES_PER_FRAME * 2;
    static const int CLOCKS_PER_SAMPLE = PC_CLOCK_HZ / SAMPLE_RATE;

    /*
     * Sound engine state
     */

    static uint8_t buffer[BUFFER_SIZE];
    static int bufferIndex;
    static bool streaming;
    static uint8_t edgeState;
    static uint32_t currentTime;

    /*
     * Private functions
     */

    static void soundOn();
    static void initStreaming(int timerChannel);
    static void timerCallback();
};

#endif // _SOUNDENGINE_H_
