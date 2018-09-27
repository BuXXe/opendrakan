/*
 * UserInterfaceProperties.h
 *
 *  Created on: Jun 12, 2018
 *      Author: zal
 */

#ifndef INCLUDE_RFL_DRAGON_USERINTERFACEPROPERTIES_H_
#define INCLUDE_RFL_DRAGON_USERINTERFACEPROPERTIES_H_

#include <odCore/rfl/RflClass.h>
#include <odCore/rfl/RflField.h>

namespace od
{

    class DragonRfl;

    class UserInterfaceProperties : public RflClass
    {
    public:

        UserInterfaceProperties(DragonRfl &rfl);

        virtual void probeFields(FieldProbe &probe) override;


    //protected:

        RflModelRef     mInnerRing;
        RflModelRef     mOuterRing;
        RflModelRef     mCrystalTop;
        RflFloat        mCrystalTopNoteOffset;
        RflModelRef     mCrystalLeft;
        RflFloat        mCrystalLeftNoteOffset;
        RflModelRef     mCrystalMiddle;
        RflFloat        mCrystalMiddleNoteOffset;
        RflModelRef     mCrystalRight;
        RflFloat        mCrystalRightNoteOffset;
        RflModelRef     mCrystalLowerLeft;
        RflFloat        mCrystalLowerLeftNoteOffset;
        RflModelRef     mCrystalLowerRight;
        RflFloat        mCrystalLowerRightNoteOffset;
        RflModelRef     mCrystalBottom;
        RflFloat        mCrystalBottomNoteOffset;
        RflModelRef     mArrowUpCrystal;
        RflModelRef     mButtonCrystal;
        RflModelRef     mWideButtonCrystal;
        RflModelRef     mNormalButtonRing;
        RflModelRef     mWideButtonRing;
        RflSoundRef     mHoverSoundLooped;
        RflSoundRef     mButtonClickSound;
        RflSoundRef     mMainGuiButtonClickSound;
        RflSoundRef     mScrollbarClickSound;
        RflSoundRef     mScrollbarReleaseSound;
        RflSoundRef     mScrollbarSlideLoopingSound;
        RflSoundRef     mNextOptionSound;
        RflSoundRef     mPreviousOptionSound;
        RflSoundRef     mMessageBoxSound;
        RflSoundRef     mExitSound;
        RflInteger      mBackgroundMusicId;
        RflInteger      mCreditsMusicId;
        RflInteger      mEasyScalePercent;
        RflInteger      mHardScalePercent;
        RflString       mIntroLevelFilename;
        RflString       mFirstLevelFilename;
    };


    OD_DEFINE_RFLCLASS_TRAITS(DragonRfl, 0x0062, "System", "User Interface Properties", UserInterfaceProperties);

}


#endif /* INCLUDE_RFL_DRAGON_USERINTERFACEPROPERTIES_H_ */