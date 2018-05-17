/*
 * Sky.h
 *
 *  Created on: 26 Feb 2018
 *      Author: zal
 */

#ifndef INCLUDE_RFL_DRAGON_SKY_H_
#define INCLUDE_RFL_DRAGON_SKY_H_

#include <osg/Vec3>
#include <osg/NodeCallback>

#include "rfl/RflClass.h"
#include "rfl/RflField.h"

namespace odRfl
{

    class DomedSky : public RflClass
    {
    public:

        DomedSky();

        virtual void probeFields(RflFieldProbe &probe) override;
        virtual void spawned(od::LevelObject &obj) override;

        void update(osg::Vec3 eyePoint);


    protected:

        RflEnumYesNo	mPrimarySky;
		RflEnum			mFollowMode;
		RflFloat		mOffsetDown;
		RflEnum			mEffects;
		RflClassRef		mLightningObject;
		RflFloat		mAveLightningPeriod;
		RflInteger		mLightningHeight;
		RflInteger		mMinLightningDist;
		RflInteger		mMaxLightningDist;
		RflFloat		mLightningWedgeAngle;
		RflClassRef		mLensFlare;
		RflInteger		mFlareElevation;
		RflInteger		mFlareDirection;


    private:

		osg::ref_ptr<od::LevelObject> mSkyObject;

    };

}

#endif /* INCLUDE_RFL_DRAGON_SKY_H_ */
