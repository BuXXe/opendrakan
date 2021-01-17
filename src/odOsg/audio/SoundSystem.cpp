/*
 * SoundSystem.cpp
 *
 *  Created on: Jan 20, 2019
 *      Author: zal
 */

#include <odOsg/audio/SoundSystem.h>

#include <exception>

#include <AL/al.h>

#include <odCore/Panic.h>
#include <odCore/ThreadUtils.h>

#include <odCore/db/Sound.h>
#include <odCore/db/MusicContainer.h>
#include <odCore/db/Segment.h>

#include <odCore/audio/music/SegmentPlayer.h>

#include <odOsg/audio/Source.h>
#include <odOsg/audio/StreamingSource.h>
#include <odOsg/audio/Buffer.h>

#ifdef USE_FLUIDSYNTH
    #include <odOsg/audio/music/FluidSynth.h>
#else
    #include <odOsg/audio/music/DummySynth.h>
    #include <odOsg/audio/music/StupidSineSynth.h>
#endif

namespace odOsg
{

    SoundSystem::SoundSystem()
    : mContext() // only support default device for now
    , mTerminateFlag(false)
    , mSegmentPlayer(nullptr)
    {
        mContext.makeCurrent();

        mWorkerThread = std::thread(&SoundSystem::_doWorkerStuff, this);
        od::ThreadUtils::setThreadName(mWorkerThread, "sound worker");
    }

    SoundSystem::~SoundSystem()
    {
        if(mMusicSource != nullptr)
        {
            mMusicSource->setBufferFillCallback(nullptr);
        }

        mTerminateFlag = true;
        if(mWorkerThread.joinable()) mWorkerThread.join();
    }

    void SoundSystem::setListenerPosition(const glm::vec3 &pos)
    {
        alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
        doErrorCheck("Could not set listener position");
    }

    void SoundSystem::setListenerOrientation(const glm::vec3 &at, const glm::vec3 &up)
    {
        float atAndUp[6] = { at.x, at.y, at.z,
                             up.x, up.y, up.z  };

        alListenerfv(AL_ORIENTATION, atAndUp);
        doErrorCheck("Could not set listener orientation");
    }

    void SoundSystem::setListenerVelocity(const glm::vec3 &v)
    {
        alListener3f(AL_VELOCITY, v.x, v.y, v.z);
        doErrorCheck("Could not set listener velocity");
    }

    std::shared_ptr<odAudio::Source> SoundSystem::createSource()
    {
        auto source = std::make_shared<Source>(*this);

        std::lock_guard<std::mutex> lock(mWorkerMutex);
        mSources.emplace_back(source);

        return source;
    }

    std::shared_ptr<odAudio::Buffer> SoundSystem::createBuffer(std::shared_ptr<odDb::Sound> sound)
    {
        return std::make_shared<Buffer>(*this, sound);
    }

    void SoundSystem::setEaxPreset(odAudio::EaxPreset preset)
    {
        OD_PANIC() << "EAX is still unimplemented";
    }

    void SoundSystem::loadMusicContainer(const od::FilePath &rrcPath)
    {
        mMusicContainer = std::make_unique<odDb::MusicContainer>(rrcPath);

#ifdef USE_FLUIDSYNTH
        auto fluidSynth = std::make_unique<FluidSynth>();
        fluidSynth->setMusicContainer(mMusicContainer.get());
        mSynth = std::move(fluidSynth);
#else
        mSynth = std::make_unique<DummySynth>();
        //mSynth = std::make_unique<StupidSineSynth>(5);
#endif
        mSegmentPlayer = std::make_unique<odAudio::SegmentPlayer>(*mSynth);

        auto musicSource = std::make_shared<StreamingSource>(*this, 128, 64, true);
        auto fillCallback = [this](int16_t *buffer, size_t size)
        {
            assert(size % 2 == 0);

            mSynth->fillInterleavedStereoBuffer(buffer, size);

            float passedTime = size/(2.0*mContext.getOutputFrequency());
            mSegmentPlayer->update(passedTime);
        };
        musicSource->setBufferFillCallback(fillCallback);
        mMusicSource = musicSource;

        std::lock_guard<std::mutex> lock(mWorkerMutex);
        mSources.emplace_back(musicSource);
    }

    void SoundSystem::playMusic(odAudio::MusicId musicId)
    {
        if(mMusicContainer == nullptr)
        {
            OD_PANIC() << "No music container loaded. Make sure to load one before trying to play music";
        }

        if(mSegmentPlayer == nullptr)
        {
            OD_PANIC() << "No segment player present. Seems like the music thread died";
        }

        auto segment = mMusicContainer->loadSegment(musicId);

        mSegmentPlayer->setSegment(segment);
        mSegmentPlayer->play();

        mMusicSource->play(0.0);
    }

    void SoundSystem::stopMusic()
    {
        mSegmentPlayer->pause();
    }

    void SoundSystem::doErrorCheck(const std::string &failmsg)
    {
        std::string alErrorMsg;
        ALenum error = alGetError();
        switch(error)
        {
        case AL_NO_ERROR:
            return;

        case AL_INVALID_NAME:
            alErrorMsg = " (invalid name)";
            break;

        case AL_INVALID_ENUM:
            alErrorMsg = " (invalid enum)";
            break;

        case AL_INVALID_OPERATION:
            alErrorMsg = " (invalid operation)";
            break;

        case AL_INVALID_VALUE:
            alErrorMsg = " (invalid value)";
            break;

        case AL_OUT_OF_MEMORY:
            alErrorMsg = " (out of memory)";
            break;

        default:
            alErrorMsg = " (unknown error)";
            break;
        }

        OD_PANIC() << failmsg << alErrorMsg;
    }

    void SoundSystem::_doWorkerStuff()
    {
        Logger::verbose() << "Started sound worker thread";

        auto lastTime = std::chrono::high_resolution_clock::now();
        while(!mTerminateFlag)
        {
            auto now = std::chrono::high_resolution_clock::now();
            float relTime = std::chrono::duration_cast<std::chrono::duration<float>>(now - lastTime).count();
            lastTime = now;

            try
            {
                std::lock_guard<std::mutex> lock(mWorkerMutex);
                for(auto &weakSource : mSources)
                {
                    auto source = weakSource.lock();
                    if(source != nullptr)
                    {
                        source->update(relTime);
                    }

                    //TODO: maybe delete expired entries here? leaving them in poses a minor memory leak
                    //  (not that severe since we don't create sources on the fly)
                }

            }catch(std::exception &e)
            {
                Logger::error() << "Error in sound worker thread: " << e.what();
                Logger::error() << "Terminating sound worker thread...";
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        Logger::verbose() << "Terminated sound worker thread";
    }

}
