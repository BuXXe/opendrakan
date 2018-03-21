/*
 * RflSky.cpp
 *
 *  Created on: 26 Feb 2018
 *      Author: zal
 */


#include "rfl/general/RflSky.h"

#include "rfl/Rfl.h"
#include "Level.h"

namespace od
{


	RflDomedSky::RflDomedSky()
	: mPrimarySky(1) // yes
	, mFollowMode(0) // original height
	, mOffsetDown(10000.0)
	, mEffects(0) // none
	, mLightningObject(AssetRef::NULL_REF)
	, mAveLightningPeriod(5.0)
	, mLightningHeight(30)
	, mMinLightningDist(10)
	, mMaxLightningDist(20)
	, mLightningWedgeAngle(90.0)
	, mLensFlare(AssetRef::NULL_REF)
	, mFlareElevation(0)
	, mFlareDirection(0)
	{
	}

    void RflDomedSky::probeFields(RflFieldProbe &probe)
    {
    	/*probe.beginCategory("Position");
    	probe.registerField(mPrimarySky, "Primary Sky");
		probe.registerField(mFollowMode, "Follow Mode");
		probe.registerField(mOffsetDown, "Offset Down");

		probe.beginCategory("Effects");
		probe.registerField(mEffects, "Effects");
		probe.registerField(mLightningObject, "Lightning Object");
		probe.registerField(mAveLightningPeriod, "Ave Lightning Period");
		probe.registerField(mLightningHeight, "Lightning Height");
		probe.registerField(mMinLightningDist, "Min Lightning Dist");
		probe.registerField(mMaxLightningDist, "Max Lightning Dist");
		probe.registerField(mLightningWedgeAngle, "Lightning Wedge Angle");
		probe.registerField(mLensFlare, "Lens Flare");
		probe.registerField(mFlareElevation, "Flare Elevation (0 - 90)");
		probe.registerField(mFlareDirection, "Flare Direction (0 - 359)");*/

		// not really clean code, but avoids the wall of text above. should we use this rather than calling the methods directly?
		probe("Position")
    		 (mPrimarySky, "Primary Sky")
			 (mFollowMode, "Follow Mode")
			 (mOffsetDown, "Offset Down")

			 ("Effects")
			 (mEffects, "Effects")
			 (mLightningObject, "Lightning Object")
		     (mAveLightningPeriod, "Ave Lightning Period")
		     (mLightningHeight, "Lightning Height")
		     (mMinLightningDist, "Min Lightning Dist")
		     (mMaxLightningDist, "Max Lightning Dist")
		     (mLightningWedgeAngle, "Lightning Wedge Angle")
		     (mLensFlare, "Lens Flare")
		     (mFlareElevation, "Flare Elevation (0 - 90)")
		     (mFlareDirection, "Flare Direction (0 - 359)");
    }

    void RflDomedSky::spawn(LevelObject &obj)
	{
    	obj.getLevel().setSkyObject(obj);
	}

    OD_REGISTER_RFL_CLASS(0x001a, "Domed Sky", RflDomedSky);

}