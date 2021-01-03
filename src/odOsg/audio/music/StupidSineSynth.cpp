/*
 * StupidSineSynth.cpp
 *
 *  Created on: Jun 28, 2019
 *      Author: zal
 */

#include <odOsg/audio/music/StupidSineSynth.h>

#include <cmath>

#include <odCore/Logger.h>

namespace odOsg
{

    StupidSineSynth::StupidSineSynth(uint8_t channel)
    : mChannel(channel)
    , mT(0)
    {
        mNotes.resize(256, false);

        double step = std::pow(2.0, 1.0/12.0);
        double pitchFreq = 440;
        size_t pitchNote = 69; // A4
        mFrequencies.resize(256, 0);
        for(size_t note = 0; note < mFrequencies.size(); ++note)
        {
            mFrequencies[note] = pitchFreq * std::pow(step, ((int)note)-((int)pitchNote));
        }
    }

    void StupidSineSynth::noteOn(uint8_t channel, uint8_t note, uint8_t velocity)
    {
        if(channel == mChannel)
        {
            mNotes[note] = true;
        }
    }

    void StupidSineSynth::noteOff(uint8_t channel, uint8_t note)
    {
        if(channel == mChannel)
        {
            mNotes[note] = false;
        }
    }

    void StupidSineSynth::pitchBend(uint8_t channel, uint16_t value)
    {
    }

    void StupidSineSynth::controllerChange(uint8_t channel, uint8_t controller, uint8_t value)
    {
    }

    void StupidSineSynth::channelPressure(uint8_t channel, uint8_t value)
    {
    }

    void StupidSineSynth::keyPressure(uint8_t channel, uint8_t key, uint8_t value)
    {
    }

    void StupidSineSynth::allNotesOff()
    {
        for(size_t i = 0; i < mNotes.size(); ++i)
        {
            mNotes[i] = false;
        }
    }

    void StupidSineSynth::fillInterleavedStereoBuffer(int16_t *buffer, size_t size)
    {
        double ampl = 0.8;
        for(size_t i = 0; i < size/2; ++i)
        {
            double sample = 0;
            size_t notes = 0;
            for(size_t note = 0; note < mNotes.size(); ++note)
            {
                if(!mNotes[note]) continue;

                double f = mFrequencies[note];
                sample += std::sin(mT*2*M_PI*f);
                notes++;
            }
            if(notes > 0) sample /= notes;
            buffer[2*i] = ampl*0xffff*0.5*sample;
            buffer[2*i+1] = buffer[2*i];
            mT += 1.0/44100;
        }
    }

    void StupidSineSynth::preloadDls(const od::Guid &dlsGuid)
    {
    }

    void StupidSineSynth::assignPreset(uint8_t channel, uint32_t bank, uint32_t patch, const od::Guid &dlsGuid)
    {
    }

    void StupidSineSynth::setChannel(uint8_t channel)
    {
        mChannel = channel;

        allNotesOff();
    }

}


