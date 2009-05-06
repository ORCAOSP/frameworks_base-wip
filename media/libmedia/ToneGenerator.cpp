/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ToneGenerator"
#include <utils/threads.h>

#include <stdio.h>
#include <math.h>
#include <utils/Log.h>
#include <sys/resource.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>
#include <cutils/properties.h>
#include "media/ToneGenerator.h"


namespace android {


// Descriptors for all available tones (See ToneGenerator::ToneDescriptor class declaration for details)
const ToneGenerator::ToneDescriptor ToneGenerator::sToneDescriptors[] = {
        { segments: {{ duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1336, 941, 0 }},
                     { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_0
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1209, 697, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_1
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1336, 697, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_2
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1477, 697, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_3
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1209, 770, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_4
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1336, 770, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_5
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1477, 770, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_6
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1209, 852, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_7
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1336, 852, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_8
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1477, 852, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_9
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1209, 941, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_S
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1477, 941, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_P
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1633, 697, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_A
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1633, 770, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                             // TONE_DTMF_B
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1633, 852, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_C
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 1633, 941, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_DTMF_D
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 425, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_SUP_DIAL
        { segments: { { duration: 500 , waveFreq: { 425, 0 }},
                      { duration: 500, waveFreq: { 0 }},
                         { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_SUP_BUSY
        { segments: { { duration: 200, waveFreq: { 425, 0 } },
                      { duration: 200, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_SUP_CONGESTION
        { segments: { { duration: 200, waveFreq: { 425, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 0,
          repeatSegment: 0 },                              // TONE_SUP_RADIO_ACK
        { segments: { { duration: 200, waveFreq: { 425, 0 }},
                      { duration: 200, waveFreq: { 0 }},
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 2,
          repeatSegment: 0 },                              // TONE_SUP_RADIO_NOTAVAIL
        { segments: { { duration: 330, waveFreq: { 950, 1400, 1800, 0 }},
                      { duration: 1000, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_SUP_ERROR
        { segments: { { duration: 200, waveFreq: { 425, 0 } },
                      { duration: 600, waveFreq: { 0 } },
                      { duration: 200, waveFreq: { 425, 0 } },
                      { duration: 3000, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_SUP_CALL_WAITING
        { segments: { { duration: 1000, waveFreq: { 425, 0 } },
                      { duration: 4000, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_SUP_RINGTONE
        { segments: { { duration: 40, waveFreq: { 400, 1200, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 0,
          repeatSegment: 0 },                              // TONE_PROP_BEEP
        { segments: { { duration: 100, waveFreq: { 1200, 0 } },
                      { duration: 100, waveFreq: { 0 }  },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 1,
          repeatSegment: 0 },                              // TONE_PROP_ACK
        { segments: { { duration: 400, waveFreq: { 300, 400, 500, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 0,
          repeatSegment: 0 },                              // TONE_PROP_NACK
        { segments: { { duration: 200, waveFreq: { 400, 1200, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 0,
          repeatSegment: 0 },                              // TONE_PROP_PROMPT
        { segments: { { duration: 40, waveFreq: { 400, 1200, 0 } },
                      { duration: 200, waveFreq: { 0 } },
                      { duration: 40, waveFreq: { 400, 1200, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 0,
          repeatSegment: 0 },                             // TONE_PROP_BEEP2
        { segments: { { duration: 250, waveFreq: { 440, 0 } },
                      { duration: 250, waveFreq: { 620, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_SUP_INTERCEPT
        { segments: { { duration: 250, waveFreq: { 440, 0 } },
                      { duration: 250, waveFreq: { 620, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 7,
          repeatSegment: 0 },                             // TONE_SUP_INTERCEPT_ABBREV
        { segments: { { duration: 250, waveFreq: { 480, 620, 0 } },
                      { duration: 250, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 7,
          repeatSegment: 0 },                             // TONE_SUP_CONGESTION_ABBREV
        { segments: { { duration: 100, waveFreq: { 350, 440, 0 } },
                      { duration: 100, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 2,
          repeatSegment: 0 },                             // TONE_SUP_CONFIRM
        { segments: { { duration: 100, waveFreq: { 480, 0 } },
                      { duration: 100, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: 3,
          repeatSegment: 0 },                              // TONE_SUP_PIP
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 350, 440, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_ANSI_DIAL
        { segments: { { duration: 500, waveFreq: { 480, 620, 0 } },
                      { duration: 500, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_ANSI_BUSY
        { segments: { { duration: 250, waveFreq: { 480, 620, 0 } },
                      { duration: 250, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_ANSI_CONGESTION
        { segments: { { duration: 300, waveFreq: { 440, 0 } },
                      { duration: 9700, waveFreq: { 0 } },
                      { duration: 100, waveFreq: { 440, 0 } },
                      { duration: 100, waveFreq: { 0 } },
                      { duration: 100, waveFreq: { 440, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 1 },                              // TONE_ANSI_CALL_WAITING
        { segments: { { duration: 2000, waveFreq: { 440, 480, 0 } },
                      { duration: 4000, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_ANSI_RINGTONE
        { segments: { { duration: ToneGenerator::TONEGEN_INF, waveFreq: { 400, 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_JAPAN_DIAL
        { segments: { { duration: 500, waveFreq: { 400, 0 } },
                      { duration: 500, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_JAPAN_BUSY
        { segments: { { duration: 1000, waveFreq: { 400, 0 } },
                      { duration: 2000, waveFreq: { 0 } },
                      { duration: 0 , waveFreq: { 0 }}},
          repeatCnt: ToneGenerator::TONEGEN_INF,
          repeatSegment: 0 },                              // TONE_JAPAN_RADIO_ACK
};

// Used by ToneGenerator::getToneForRegion() to convert user specified supervisory tone type
// to actual tone for current region.
const unsigned char ToneGenerator::sToneMappingTable[NUM_REGIONS-1][NUM_SUP_TONES] = {
        {   // ANSI
            TONE_ANSI_DIAL,             // TONE_SUP_DIAL
            TONE_ANSI_BUSY,             // TONE_SUP_BUSY
            TONE_ANSI_CONGESTION,       // TONE_SUP_CONGESTION
            TONE_SUP_RADIO_ACK,         // TONE_SUP_RADIO_ACK
            TONE_SUP_RADIO_NOTAVAIL,    // TONE_SUP_RADIO_NOTAVAIL
            TONE_SUP_ERROR,             // TONE_SUP_ERROR
            TONE_ANSI_CALL_WAITING,     // TONE_SUP_CALL_WAITING
            TONE_ANSI_RINGTONE          // TONE_SUP_RINGTONE
        },
        {   // JAPAN
            TONE_JAPAN_DIAL,             // TONE_SUP_DIAL
            TONE_JAPAN_BUSY,             // TONE_SUP_BUSY
            TONE_SUP_CONGESTION,         // TONE_SUP_CONGESTION
            TONE_JAPAN_RADIO_ACK,        // TONE_SUP_RADIO_ACK
            TONE_SUP_RADIO_NOTAVAIL,     // TONE_SUP_RADIO_NOTAVAIL
            TONE_SUP_ERROR,              // TONE_SUP_ERROR
            TONE_SUP_CALL_WAITING,       // TONE_SUP_CALL_WAITING
            TONE_SUP_RINGTONE            // TONE_SUP_RINGTONE
        }
};


////////////////////////////////////////////////////////////////////////////////
//                           ToneGenerator class Implementation
////////////////////////////////////////////////////////////////////////////////


//---------------------------------- public methods ----------------------------


////////////////////////////////////////////////////////////////////////////////
//
//    Method:        ToneGenerator::ToneGenerator()
//
//    Description:    Constructor. Initializes the tone sequencer, intantiates required sine wave
//        generators, instantiates output audio track.
//
//    Input:
//        toneType:        Type of tone generated (values in enum tone_type)
//        streamType:        Type of stream used for tone playback (enum AudioTrack::stream_type)
//        volume:            volume applied to tone (0.0 to 1.0)
//
//    Output:
//        none
//
////////////////////////////////////////////////////////////////////////////////
ToneGenerator::ToneGenerator(int streamType, float volume) {

    LOGV("ToneGenerator constructor: streamType=%d, volume=%f\n", streamType, volume);

    mState = TONE_IDLE;

    if (AudioSystem::getOutputSamplingRate(&mSamplingRate, streamType) != NO_ERROR) {
        LOGE("Unable to marshal AudioFlinger");
        return;
    }
    mStreamType = streamType;
    mVolume = volume;
    mpAudioTrack = 0;
    mpToneDesc = 0;
    mpNewToneDesc = 0;
    // Generate tone by chunks of 20 ms to keep cadencing precision
    mProcessSize = (mSamplingRate * 20) / 1000;

    char value[PROPERTY_VALUE_MAX];
    property_get("gsm.operator.iso-country", value, "");
    if (strcmp(value,"us") == 0 ||
        strcmp(value,"ca") == 0) {
        mRegion = ANSI;
    } else if (strcmp(value,"jp") == 0) {
        mRegion = JAPAN;
    } else {
        mRegion = CEPT;
    }

    if (initAudioTrack()) {
        LOGV("ToneGenerator INIT OK, time: %d\n", (unsigned int)(systemTime()/1000000));
    } else {
        LOGV("!!!ToneGenerator INIT FAILED!!!\n");
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//    Method:        ToneGenerator::~ToneGenerator()
//
//    Description:    Destructor. Stop sound playback and delete audio track if
//      needed and delete sine wave generators.
//
//    Input:
//        none
//
//    Output:
//        none
//
////////////////////////////////////////////////////////////////////////////////
ToneGenerator::~ToneGenerator() {
    LOGV("ToneGenerator destructor\n");

    if (mpAudioTrack) {
        stopTone();
        LOGV("Delete Track: %p\n", mpAudioTrack);
        delete mpAudioTrack;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//    Method:        ToneGenerator::startTone()
//
//    Description:    Starts tone playback.
//
//    Input:
//        none
//
//    Output:
//        none
//
////////////////////////////////////////////////////////////////////////////////
bool ToneGenerator::startTone(int toneType) {
    bool lResult = false;

    if (toneType >= NUM_TONES)
        return lResult;

    if (mState == TONE_IDLE) {
        LOGV("startTone: try to re-init AudioTrack");
        if (!initAudioTrack()) {
            return lResult;
        }
    }

    LOGV("startTone\n");

    mLock.lock();

    // Get descriptor for requested tone
    toneType = getToneForRegion(toneType);
    mpNewToneDesc = &sToneDescriptors[toneType];

    if (mState == TONE_INIT) {
        if (prepareWave()) {
            LOGV("Immediate start, time %d\n", (unsigned int)(systemTime()/1000000));
            lResult = true;
            mState = TONE_STARTING;
            mLock.unlock();
            mpAudioTrack->start();
            mLock.lock();
            if (mState == TONE_STARTING) {
                LOGV("Wait for start callback");
                status_t lStatus = mWaitCbkCond.waitRelative(mLock, seconds(1));
                if (lStatus != NO_ERROR) {
                    LOGE("--- Immediate start timed out, status %d", lStatus);
                    mState = TONE_IDLE;
                    lResult = false;
                }
            }
        } else {
            mState == TONE_IDLE;
        }
    } else {
        LOGV("Delayed start\n");

        mState = TONE_RESTARTING;
        status_t lStatus = mWaitCbkCond.waitRelative(mLock, seconds(1));
        if (lStatus == NO_ERROR) {
            if (mState != TONE_IDLE) {
                lResult = true;
            }
            LOGV("cond received");
        } else {
            LOGE("--- Delayed start timed out, status %d", lStatus);
            mState = TONE_IDLE;
        }
    }
    mLock.unlock();

    LOGV_IF(lResult, "Tone started, time %d\n", (unsigned int)(systemTime()/1000000));
    LOGW_IF(!lResult, "Tone start failed!!!, time %d\n", (unsigned int)(systemTime()/1000000));

    return lResult;
}

////////////////////////////////////////////////////////////////////////////////
//
//    Method:        ToneGenerator::stopTone()
//
//    Description:    Stops tone playback.
//
//    Input:
//        none
//
//    Output:
//        none
//
////////////////////////////////////////////////////////////////////////////////
void ToneGenerator::stopTone() {
    LOGV("stopTone");

    mLock.lock();
    if (mState == TONE_PLAYING || mState == TONE_STARTING || mState == TONE_RESTARTING) {
        mState = TONE_STOPPING;
        LOGV("waiting cond");
        status_t lStatus = mWaitCbkCond.waitRelative(mLock, seconds(1));
        if (lStatus == NO_ERROR) {
            LOGV("track stop complete, time %d", (unsigned int)(systemTime()/1000000));
        } else {
            LOGE("--- Stop timed out");
            mState = TONE_IDLE;
            mpAudioTrack->stop();
        }
    }

    clearWaveGens();

    mLock.unlock();
}

//---------------------------------- private methods ---------------------------




////////////////////////////////////////////////////////////////////////////////
//
//    Method:        ToneGenerator::initAudioTrack()
//
//    Description:    Allocates and configures AudioTrack used for PCM output.
//
//    Input:
//        none
//
//    Output:
//        none
//
////////////////////////////////////////////////////////////////////////////////
bool ToneGenerator::initAudioTrack() {

    if (mpAudioTrack) {
        delete mpAudioTrack;
        mpAudioTrack = 0;
    }

   // Open audio track in mono, PCM 16bit, default sampling rate, default buffer size
    mpAudioTrack
            = new AudioTrack(mStreamType, 0, AudioSystem::PCM_16_BIT, 1, 0, 0, audioCallback, this, 0);

    if (mpAudioTrack == 0) {
        LOGE("AudioTrack allocation failed");
        goto initAudioTrack_exit;
    }
    LOGV("Create Track: %p\n", mpAudioTrack);

    if (mpAudioTrack->initCheck() != NO_ERROR) {
        LOGE("AudioTrack->initCheck failed");
        goto initAudioTrack_exit;
    }

    mpAudioTrack->setVolume(mVolume, mVolume);

    mState = TONE_INIT;

    return true;

initAudioTrack_exit:

    // Cleanup
    if (mpAudioTrack) {
        LOGV("Delete Track I: %p\n", mpAudioTrack);
        delete mpAudioTrack;
        mpAudioTrack = 0;
    }

    return false;
}


////////////////////////////////////////////////////////////////////////////////
//
//    Method:        ToneGenerator::audioCallback()
//
//    Description:    AudioTrack callback implementation. Generates a block of
//        PCM samples
//        and manages tone generator sequencer: tones pulses, tone duration...
//
//    Input:
//        user    reference (pointer to our ToneGenerator)
//        info    audio buffer descriptor
//
//    Output:
//        returned value: always true.
//
////////////////////////////////////////////////////////////////////////////////
void ToneGenerator::audioCallback(int event, void* user, void *info) {
    
    if (event != AudioTrack::EVENT_MORE_DATA) return;
    
    const AudioTrack::Buffer *buffer = static_cast<const AudioTrack::Buffer *>(info);
    ToneGenerator *lpToneGen = static_cast<ToneGenerator *>(user);
    short *lpOut = buffer->i16;
    unsigned int lNumSmp = buffer->size/sizeof(short);
    const ToneDescriptor *lpToneDesc = lpToneGen->mpToneDesc;

    if (buffer->size == 0) return;


    // Clear output buffer: WaveGenerator accumulates into lpOut buffer
    memset(lpOut, 0, buffer->size);

    while (lNumSmp) {
        unsigned int lReqSmp = lNumSmp < lpToneGen->mProcessSize*2 ? lNumSmp : lpToneGen->mProcessSize;
        unsigned int lGenSmp;
        unsigned int lWaveCmd = WaveGenerator::WAVEGEN_CONT;
        bool lSignal = false;
 
        lpToneGen->mLock.lock();

        // Update pcm frame count and end time (current time at the end of this process)
        lpToneGen->mTotalSmp += lReqSmp;
    
        // Update tone gen state machine and select wave gen command
        switch (lpToneGen->mState) {
        case TONE_PLAYING:
            lWaveCmd = WaveGenerator::WAVEGEN_CONT;
            break;
        case TONE_STARTING:
            LOGV("Starting Cbk");
    
            lWaveCmd = WaveGenerator::WAVEGEN_START;
            break;
        case TONE_STOPPING:
        case TONE_RESTARTING:
            LOGV("Stop/restart Cbk");
    
            lWaveCmd = WaveGenerator::WAVEGEN_STOP;
            lpToneGen->mNextSegSmp = TONEGEN_INF; // forced to skip state machine management below
            break;
        default:
            LOGV("Extra Cbk");
            // Force loop exit
            lNumSmp = 0;
            goto audioCallback_EndLoop;
        }
        
    
        // Exit if tone sequence is over
        if (lpToneDesc->segments[lpToneGen->mCurSegment].duration == 0) {
            if (lpToneGen->mState == TONE_PLAYING) {
                lpToneGen->mState = TONE_STOPPING;            
            }
            goto audioCallback_EndLoop;
        }
    
        if (lpToneGen->mTotalSmp > lpToneGen->mNextSegSmp) {
            // Time to go to next sequence segment
    
            LOGV("End Segment, time: %d\n", (unsigned int)(systemTime()/1000000));
    
            lGenSmp = lReqSmp;

            // If segment,  ON -> OFF transition : ramp volume down
            if (lpToneDesc->segments[lpToneGen->mCurSegment].waveFreq[0] != 0) {
                lWaveCmd = WaveGenerator::WAVEGEN_STOP;
                unsigned int lFreqIdx = 0;
                unsigned short lFrequency = lpToneDesc->segments[lpToneGen->mCurSegment].waveFreq[lFreqIdx];

                while (lFrequency != 0) {
                    WaveGenerator *lpWaveGen = lpToneGen->mWaveGens.valueFor(lFrequency);
                    lpWaveGen->getSamples(lpOut, lGenSmp, lWaveCmd);
                    lFrequency = lpToneDesc->segments[lpToneGen->mCurSegment].waveFreq[++lFreqIdx];
                }
                LOGV("ON->OFF, lGenSmp: %d, lReqSmp: %d\n", lGenSmp, lReqSmp);
            }

            // Go to next segment
            lpToneGen->mCurSegment++;

            // Handle loop if last segment reached
            if (lpToneDesc->segments[lpToneGen->mCurSegment].duration == 0) {
                LOGV("Last Seg: %d\n", lpToneGen->mCurSegment);
    
                // Pre increment loop count and restart if total count not reached. Stop sequence otherwise
                if (++lpToneGen->mCurCount <= lpToneDesc->repeatCnt) {
                    LOGV("Repeating Count: %d\n", lpToneGen->mCurCount);
    
                    lpToneGen->mCurSegment = lpToneDesc->repeatSegment;
                    if (lpToneDesc->segments[lpToneDesc->repeatSegment].waveFreq[0] != 0) {
                        lWaveCmd = WaveGenerator::WAVEGEN_START;
                    }
    
                    LOGV("New segment %d, Next Time: %d\n", lpToneGen->mCurSegment,
                            (lpToneGen->mNextSegSmp*1000)/lpToneGen->mSamplingRate);
    
                } else {
                    lGenSmp = 0;
                    LOGV("End repeat, time: %d\n", (unsigned int)(systemTime()/1000000));
                }
            } else {
                LOGV("New segment %d, Next Time: %d\n", lpToneGen->mCurSegment,
                        (lpToneGen->mNextSegSmp*1000)/lpToneGen->mSamplingRate);
                if (lpToneDesc->segments[lpToneGen->mCurSegment].waveFreq[0] != 0) {
                    // If next segment is not silent,  OFF -> ON transition : reset wave generator
                    lWaveCmd = WaveGenerator::WAVEGEN_START;

                    LOGV("OFF->ON, lGenSmp: %d, lReqSmp: %d\n", lGenSmp, lReqSmp);
                } else {
                    lGenSmp = 0;
                }
            }
    
            // Update next segment transition position. No harm to do it also for last segment as lpToneGen->mNextSegSmp won't be used any more
            lpToneGen->mNextSegSmp
                    += (lpToneDesc->segments[lpToneGen->mCurSegment].duration * lpToneGen->mSamplingRate) / 1000;
    
        } else {
            // Inside a segment keep tone ON or OFF
            if (lpToneDesc->segments[lpToneGen->mCurSegment].waveFreq[0] == 0) {
                lGenSmp = 0;  // If odd segment, tone is currently OFF
            } else {
                lGenSmp = lReqSmp;  // If event segment, tone is currently ON
            }
        }
    
        if (lGenSmp) {
            // If samples must be generated, call all active wave generators and acumulate waves in lpOut
            unsigned int lFreqIdx = 0;
            unsigned short lFrequency = lpToneDesc->segments[lpToneGen->mCurSegment].waveFreq[lFreqIdx];
    
            while (lFrequency != 0) {
                WaveGenerator *lpWaveGen = lpToneGen->mWaveGens.valueFor(lFrequency);
                lpWaveGen->getSamples(lpOut, lGenSmp, lWaveCmd);
                lFrequency = lpToneDesc->segments[lpToneGen->mCurSegment].waveFreq[++lFreqIdx];
            }
        }
        
        lNumSmp -= lReqSmp;
        lpOut += lReqSmp;
    
audioCallback_EndLoop:
    
        switch (lpToneGen->mState) {
        case TONE_RESTARTING:
            LOGV("Cbk restarting track\n");
            if (lpToneGen->prepareWave()) {
                lpToneGen->mState = TONE_STARTING;
            } else {
                LOGW("Cbk restarting prepareWave() failed\n");
                lpToneGen->mState = TONE_IDLE;
                lpToneGen->mpAudioTrack->stop();
                // Force loop exit
                lNumSmp = 0;
            }
            lSignal = true;
            break;
        case TONE_STOPPING:
            lpToneGen->mState = TONE_INIT;
            LOGV("Cbk Stopping track\n");
            lSignal = true;
            lpToneGen->mpAudioTrack->stop();
            
            // Force loop exit
            lNumSmp = 0;
            break;
        case TONE_STARTING:
            LOGV("Cbk starting track\n");
            lpToneGen->mState = TONE_PLAYING;
            lSignal = true;
           break;
        default:
            break;
        }

        if (lSignal)
            lpToneGen->mWaitCbkCond.signal();
        lpToneGen->mLock.unlock();
    }
}


////////////////////////////////////////////////////////////////////////////////
//
//    Method:        ToneGenerator::prepareWave()
//
//    Description:    Prepare wave generators and reset tone sequencer state machine.
//      mpNewToneDesc must have been initialized before calling this function.
//    Input:
//        none
//
//    Output:
//        returned value:   true if wave generators have been created, false otherwise
//
////////////////////////////////////////////////////////////////////////////////
bool ToneGenerator::prepareWave() {
    unsigned int segmentIdx = 0;

    if (!mpNewToneDesc) {
        return false;
    }

    // Remove existing wave generators if any
    clearWaveGens();

    mpToneDesc = mpNewToneDesc;

    while (mpToneDesc->segments[segmentIdx].duration) {
        // Get total number of sine waves: needed to adapt sine wave gain.
        unsigned int lNumWaves = numWaves(segmentIdx);
        unsigned int freqIdx = 0;
        unsigned int frequency = mpToneDesc->segments[segmentIdx].waveFreq[freqIdx];
        while (frequency) {
            // Instantiate a wave generator if  ot already done for this frequency
            if (mWaveGens.indexOfKey(frequency) == NAME_NOT_FOUND) {
                ToneGenerator::WaveGenerator *lpWaveGen =
                        new ToneGenerator::WaveGenerator((unsigned short)mSamplingRate,
                                frequency,
                                TONEGEN_GAIN/lNumWaves);
                if (lpWaveGen == 0) {
                    goto prepareWave_exit;
                }
                mWaveGens.add(frequency, lpWaveGen);
            }
            frequency = mpNewToneDesc->segments[segmentIdx].waveFreq[++freqIdx];
        }
        segmentIdx++;
    }

    // Initialize tone sequencer
    mTotalSmp = 0;
    mCurSegment = 0;
    mCurCount = 0;
    if (mpToneDesc->segments[0].duration == TONEGEN_INF) {
        mNextSegSmp = TONEGEN_INF;
    } else{
        mNextSegSmp = (mpToneDesc->segments[0].duration * mSamplingRate) / 1000;
    }

    return true;

prepareWave_exit:

    clearWaveGens();

    return false;
}


////////////////////////////////////////////////////////////////////////////////
//
//    Method:        ToneGenerator::numWaves()
//
//    Description:    Count number of sine waves needed to generate a tone segment (e.g 2 for DTMF).
//
//    Input:
//        segmentIdx        tone segment index
//
//    Output:
//        returned value:    nummber of sine waves
//
////////////////////////////////////////////////////////////////////////////////
unsigned int ToneGenerator::numWaves(unsigned int segmentIdx) {
    unsigned int lCnt = 0;

    if (mpToneDesc->segments[segmentIdx].duration) {
        while (mpToneDesc->segments[segmentIdx].waveFreq[lCnt]) {
            lCnt++;
        }
        lCnt++;
    }

    return lCnt;
}


////////////////////////////////////////////////////////////////////////////////
//
//    Method:        ToneGenerator::clearWaveGens()
//
//    Description:    Removes all wave generators.
//
//    Input:
//        none
//
//    Output:
//        none
//
////////////////////////////////////////////////////////////////////////////////
void ToneGenerator::clearWaveGens() {
    LOGV("Clearing mWaveGens:");

    for (size_t lIdx = 0; lIdx < mWaveGens.size(); lIdx++) {
        delete mWaveGens.valueAt(lIdx);
    }
    mWaveGens.clear();
}

////////////////////////////////////////////////////////////////////////////////
//
//    Method:       ToneGenerator::getToneForRegion()
//
//    Description:  Get correct ringtone type according to current region.
//      The corrected ring tone type is the tone descriptor index in sToneDescriptors[].
//
//    Input:
//        none
//
//    Output:
//        none
//
////////////////////////////////////////////////////////////////////////////////
int ToneGenerator::getToneForRegion(int toneType) {
    int regionTone;

    if (mRegion == CEPT || toneType < FIRST_SUP_TONE || toneType > LAST_SUP_TONE) {
        regionTone = toneType;
    } else {
        regionTone = sToneMappingTable[mRegion][toneType - FIRST_SUP_TONE];
    }

    LOGV("getToneForRegion, tone %d, region %d, regionTone %d", toneType, mRegion, regionTone);

    return regionTone;
}


////////////////////////////////////////////////////////////////////////////////
//                WaveGenerator::WaveGenerator class    Implementation
////////////////////////////////////////////////////////////////////////////////

//---------------------------------- public methods ----------------------------

////////////////////////////////////////////////////////////////////////////////
//
//    Method:        WaveGenerator::WaveGenerator()
//
//    Description:    Constructor.
//
//    Input:
//        samplingRate:    Output sampling rate in Hz
//        frequency:       Frequency of the sine wave to generate in Hz
//        volume:          volume (0.0 to 1.0)
//
//    Output:
//        none
//
////////////////////////////////////////////////////////////////////////////////
ToneGenerator::WaveGenerator::WaveGenerator(unsigned short samplingRate,
        unsigned short frequency, float volume) {
    double d0;
    double F_div_Fs;  // frequency / samplingRate

    F_div_Fs = frequency / (double)samplingRate;
    d0 = - (float)GEN_AMP * sin(2 * M_PI * F_div_Fs);
    mS2_0 = (short)d0;
    mS1 = 0;
    mS2 = mS2_0;

    mAmplitude_Q15 = (short)(32767. * 32767. * volume / GEN_AMP);
    // take some margin for amplitude fluctuation
    if (mAmplitude_Q15 > 32500)
        mAmplitude_Q15 = 32500;

    d0 = 32768.0 * cos(2 * M_PI * F_div_Fs);  // Q14*2*cos()
    if (d0 > 32767)
        d0 = 32767;
    mA1_Q14 = (short) d0;

    LOGV("WaveGenerator init, mA1_Q14: %d, mS2_0: %d, mAmplitude_Q15: %d\n",
            mA1_Q14, mS2_0, mAmplitude_Q15);
}

////////////////////////////////////////////////////////////////////////////////
//
//    Method:        WaveGenerator::~WaveGenerator()
//
//    Description:    Destructor.
//
//    Input:
//        none
//
//    Output:
//        none
//
////////////////////////////////////////////////////////////////////////////////
ToneGenerator::WaveGenerator::~WaveGenerator() {
}

////////////////////////////////////////////////////////////////////////////////
//
//    Method:        WaveGenerator::getSamples()
//
//    Description:    Generates count samples of a sine wave and accumulates
//        result in outBuffer.
//
//    Input:
//        outBuffer:      Output buffer where to accumulate samples.
//        count:          number of samples to produce.
//        command:        special action requested (see enum gen_command).
//
//    Output:
//        none
//
////////////////////////////////////////////////////////////////////////////////
void ToneGenerator::WaveGenerator::getSamples(short *outBuffer,
        unsigned int count, unsigned int command) {
    long lS1, lS2;
    long lA1, lAmplitude;
    long Sample;  // current sample

    // init local
    if (command == WAVEGEN_START) {
        lS1 = (long)0;
        lS2 = (long)mS2_0;
    } else {
        lS1 = (long)mS1;
        lS2 = (long)mS2;
    }
    lA1 = (long)mA1_Q14;
    lAmplitude = (long)mAmplitude_Q15;

    if (command == WAVEGEN_STOP) {
        lAmplitude <<= 16;
        if (count == 0) {
            return;
        }
        long dec = lAmplitude/count;
        // loop generation
        while (count--) {
            Sample = ((lA1 * lS1) >> S_Q14) - lS2;
            // shift delay
            lS2 = lS1;
            lS1 = Sample;
            Sample = ((lAmplitude>>16) * Sample) >> S_Q15;
            *(outBuffer++) += (short)Sample;  // put result in buffer
            lAmplitude -= dec;
        }
    } else {
        // loop generation
        while (count--) {
            Sample = ((lA1 * lS1) >> S_Q14) - lS2;
            // shift delay
            lS2 = lS1;
            lS1 = Sample;
            Sample = (lAmplitude * Sample) >> S_Q15;
            *(outBuffer++) += (short)Sample;  // put result in buffer
        }
    }

    // save status
    mS1 = (short)lS1;
    mS2 = (short)lS2;
}

}  // end namespace android

