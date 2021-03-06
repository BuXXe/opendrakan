/*
 * PushableObject.cpp
 *
 *  Created on: 18 Apr 2018
 *      Author: zal
 */

#include "rfl/dragon/PushableObject.h"

#include "rfl/Rfl.h"
#include "Level.h"
#include "Engine.h"
#include "physics/PhysicsManager.h"
#include "LevelObject.h"

namespace odRfl
{

    PushableObject::PushableObject()
    : mWaitForTrigger(false)
    , mBurnable(false)
    , mBounceSounds({})
    , mLoopedRollSound(od::AssetRef::NULL_REF)
    , mDetectOtherPushables(true)
    , mSpecialEffect(od::AssetRef::NULL_REF)
    , mWaterEffects(1)
    , mGory(false)
    , mPushMode(0)
    , mPivotPoint(0)
    , mFriction(0.1)
    , mElasticity(1.5)
    , mRigidBody(false)
    , mLifeTime(0.0)
    , mHealth(-1)
    , mExplosionGenerator(od::AssetRef::NULL_REF)
    , mDamage(0)
    , mDamagePlayer(0)
    {
    }

    void PushableObject::probeFields(RflFieldProbe &probe)
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

    void PushableObject::spawned(od::LevelObject &obj)
    {
    	if(obj.getClass()->getModel() != nullptr && obj.getClass()->getModel()->getModelBounds() != nullptr)
    	{
    		obj.getLevel().getPhysicsManager().addObject(obj, 0.8);
    	}
    }

    void PushableObject::despawned(od::LevelObject &obj)
    {
    	obj.getLevel().getPhysicsManager().removeObject(obj);
    }

    OD_REGISTER_RFL_CLASS(0x0010, "Pushable Object", PushableObject);

}


