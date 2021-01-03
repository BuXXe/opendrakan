/*
 * PushableObject.cpp
 *
 *  Created on: 18 Apr 2018
 *      Author: zal
 */

#include <dragonRfl/classes/PushableObject.h>

#include <dragonRfl/RflDragon.h>
#include <odCore/rfl/Rfl.h>
#include <odCore/Level.h>
#include <odCore/LevelObject.h>

namespace dragonRfl
{

    PushableObject::PushableObject()
    : mWaitForTrigger(false)
    , mBurnable(false)
    , mBounceSounds({})
    , mLoopedRollSound(odDb::AssetRef::NULL_REF)
    , mDetectOtherPushables(true)
    , mSpecialEffect(odDb::AssetRef::NULL_REF)
    , mWaterEffects(1)
    , mGory(false)
    , mPushMode(0)
    , mPivotPoint(0)
    , mFriction(0.1)
    , mElasticity(1.5)
    , mRigidBody(false)
    , mLifeTime(0.0)
    , mHealth(-1)
    , mExplosionGenerator(odDb::AssetRef::NULL_REF)
    , mDamage(0)
    , mDamagePlayer(0)
    {
    }

    void PushableObject::probeFields(odRfl::FieldProbe &probe)
    {
        probe
            ("Pushable Object")
                (mWaitForTrigger, "Wait For Trigger?")
                (mBurnable, "Burnable?")
                (mBounceSounds, "Bounce Sound")
                (mLoopedRollSound, "Looped Roll Sound")
                (mDetectOtherPushables, "Detect Other Pushables")
                (mSpecialEffect, "Special Effect")
                (mWaterEffects, "Water Effects")
                (mGory, "Gory?")
            ("Physics")
                (mPushMode, "Push Mode")
                (mPivotPoint, "Pivot Point")
                (mFriction, "Friction")
                (mElasticity, "Elasticity")
                (mRigidBody, "Rigid Body")
            ("Life + Death")
                (mLifeTime, "LifeTime (0=Forever)")
                (mHealth, "Health (-1 = immortal)")
                (mExplosionGenerator, "Explosion Generator")
                (mDamage, "Damage (per lu/s)")
                (mDamagePlayer, "Damage Player");
    }

    void PushableObject::onSpawned()
    {
    }

    void PushableObject::onDespawned()
    {
    }

}


