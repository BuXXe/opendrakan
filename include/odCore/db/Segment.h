/*
 * Segment.h
 *
 *  Created on: Jun 15, 2019
 *      Author: zal
 */

#ifndef INCLUDE_ODCORE_AUDIO_MUSIC_SEGMENT_H_
#define INCLUDE_ODCORE_AUDIO_MUSIC_SEGMENT_H_

#include <vector>
#include <memory>
#include <set>

#include <odCore/RiffReader.h>
#include <odCore/Guid.h>

namespace odDb
{
    class Band
    {
    public:

        struct Instrument
        {
            uint32_t patch;
            uint32_t assignPatch;
            uint32_t noteRanges[4];
            uint32_t pChannel;
            uint32_t flags;
            uint8_t  pan;
            uint8_t  volume;
            int16_t  transpose;
            uint32_t channelPriority;
            int16_t  pitchBendRange;

            od::Guid dlsGuid;
        };

        Band(od::RiffReader rr);

        inline const std::vector<Instrument> &getInstruments() const { return mInstruments; }
        inline const std::set<od::Guid> &getDlsGuids() const { return mDlsGuids; }


    private:

        std::vector<Instrument> mInstruments;

        std::set<od::Guid> mDlsGuids;

    };

    /**
     * @brief Data model for a styleless DirectMusic segment.
     *
     * This won't load styles. It is only useful for segments that store all music data in a sequence track (like the ones used by Drakan).
     *
     * All tracks except MIDI events are sorted by (starting) time after being loaded.
     */
    class Segment
    {
    public:

        typedef uint32_t music_time_t;
        typedef uint64_t ref_time_t;

        struct MidiEvent
        {
            static const size_t STRUCT_BYTES = 17;

            uint32_t startTime; // MUSIC_TIME
            uint32_t duration;  // MUSIC_TIME
            uint32_t channelIndex;
            int16_t timeOffset;
            uint8_t midiStatus;
            uint8_t midiData1;
            uint8_t midiData2;
        };
        static_assert(sizeof(MidiEvent) >= MidiEvent::STRUCT_BYTES, "Invalid struct bytes");

        struct MidiCurve
        {
            static const size_t STRUCT_BYTES = 28;

            music_time_t startTime;
            music_time_t duration;
            music_time_t resetDuration;
            uint32_t channelIndex;
            int16_t timeOffset;
            int16_t startValue;
            int16_t endValue;
            int16_t resetValue;
            uint8_t type;
            uint8_t curveShape;
            uint8_t ccData;
            uint8_t flags;
            // DX8. these don't appear in drakan SGTs
            //uint16_t paramType;
            //uint16_t mergeIndex;
        };
        static_assert(sizeof(MidiCurve) >= MidiCurve::STRUCT_BYTES, "Invalid struct bytes");

        struct BandEvent
        {
            music_time_t time;
            std::unique_ptr<Band> band;
        };

        struct TempoEvent
        {
            static const size_t STRUCT_BYTES = 12;

            music_time_t time;
            double tempo; // bpm
        };

        Segment(od::RiffReader rr);

        typedef std::vector<MidiEvent> MidiEventVector;
        typedef std::vector<MidiCurve> CurveVector;
        typedef std::vector<TempoEvent> TempoVector;
        typedef std::vector<BandEvent> BandVector;

        inline od::Guid getGuid() const { return mGuid; }
        inline const MidiEventVector &getMidiEvents() const { return mMidiEvents; }
        inline const CurveVector &getMidiCurves() const { return mMidiCurves; }
        inline const TempoVector &getTempoEvents() const { return mTempoEvents; }
        inline const BandVector &getBandEvents() const { return mBandEvents; }
        inline const std::set<od::Guid> &getDlsGuids() const { return mDlsGuids; }


    private:

        void _load(od::RiffReader rr);
        void _loadHeader(od::RiffReader rr);
        void _loadTracklist(od::RiffReader rr);
        void _loadSequenceTrack(od::RiffReader rr);
        void _loadBandTrack(od::RiffReader rr);
        void _loadTempoTrack(od::RiffReader rr);

        uint32_t     mNumberOfRepeats;
        music_time_t mLength;
        music_time_t mPlayStart;
        music_time_t mLoopStart;
        music_time_t mLoopEnd;
        uint32_t     mResolution;
        ref_time_t   mRefTimeLength;
        uint32_t     mFlags;
        uint32_t     mReserved;
        ref_time_t   mRefTimeLoopStart;
        ref_time_t   mRefTimeLoopEnd;
        ref_time_t   mRefTimePlayStart;

        od::Guid mGuid;

        MidiEventVector mMidiEvents;
        CurveVector mMidiCurves;
        BandVector mBandEvents;
        TempoVector mTempoEvents;

        std::set<od::Guid> mDlsGuids;

    };

}


#endif /* INCLUDE_ODCORE_AUDIO_MUSIC_SEGMENT_H_ */
