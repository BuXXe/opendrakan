/*
 * SoundSystem.h
 *
 *  Created on: Jan 20, 2019
 *      Author: zal
 */

#ifndef SRC_ODCORE_AUDIO_SOUNDSYSTEM_H_
#define SRC_ODCORE_AUDIO_SOUNDSYSTEM_H_

#include <memory>
#include <glm/vec3.hpp>

#include <odCore/FilePath.h>

#include <odCore/audio/EaxPresets.h>

namespace odDb
{
    class Sound;
}

namespace odAudio
{

    class Source;
    class Buffer;

    typedef uint32_t MusicId;

    /**
     * @brief Abstract interface for a sound system implementation.
     */
    class SoundSystem
    {
    public:

        virtual ~SoundSystem() = default;

        virtual void setListenerPosition(const glm::vec3 &pos) = 0;
        virtual void setListenerOrientation(const glm::vec3 &at, const glm::vec3 &up) = 0;
        virtual void setListenerVelocity(const glm::vec3 &v) = 0;

        virtual std::shared_ptr<Source> createSource() = 0;
        virtual std::shared_ptr<Buffer> createBuffer(std::shared_ptr<odDb::Sound> sound) = 0;

        virtual void setEaxPreset(EaxPreset preset) = 0;

        virtual void loadMusicContainer(const od::FilePath &rrcPath) = 0;
        virtual void playMusic(MusicId musicId) = 0;
        virtual void stopMusic() = 0;

    };

}


#endif /* SRC_ODCORE_AUDIO_SOUNDSYSTEM_H_ */
