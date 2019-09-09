/*
 * AnimationDemo.h
 *
 *  Created on: 15 Jun 2018
 *      Author: zal
 */

#ifndef INCLUDE_RFL_DRAGON_ANIMATIONDEMO_H_
#define INCLUDE_RFL_DRAGON_ANIMATIONDEMO_H_

#include <memory>

#include <odCore/rfl/Class.h>
#include <odCore/rfl/Field.h>
#include <odCore/rfl/AssetRefField.h>

#include <odCore/anim/SkeletonAnimationPlayer.h>

#include <odCore/render/Handle.h>

namespace dragonRfl
{

    class DragonRfl;

    class AnimationDemo : public odRfl::LevelObjectClassBase
    {
    public:

        AnimationDemo();

        virtual void probeFields(odRfl::FieldProbe &probe) override;

        virtual void onSpawned() override;
        virtual void onUpdate(float relTime) override;


    protected:

        odRfl::AnimRefArray mAnimations;
        odRfl::Float        mSwitchPeriodSeconds;


    private:

        bool mFirstUpdate;
        float mRunningTime;
        size_t mCurrentAnimIndex;

        std::unique_ptr<odAnim::SkeletonAnimationPlayer> mPlayer;

        od::RefPtr<odRender::Handle> mRenderHandle;

    };

}

ODRFL_DEFINE_CLASS_BASE(dragonRfl::AnimationDemo, 0x000c, "Debug", "Animation Demo");

#endif /* INCLUDE_RFL_DRAGON_ANIMATIONDEMO_H_ */
