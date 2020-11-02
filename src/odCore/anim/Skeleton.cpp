/*
 * Skeleton.cpp
 *
 *  Created on: 14 Mar 2018
 *      Author: zal
 */

#include <odCore/anim/Skeleton.h>

#include <algorithm>

#include <glm/matrix.hpp>

#include <odCore/Exception.h>

#include <odCore/render/Rig.h>

namespace odAnim
{

    Skeleton::Bone::Bone(Skeleton &skeleton, int32_t jointIndex)
    : mSkeleton(skeleton)
    , mParent(nullptr)
    , mJointIndex(jointIndex)
    {
    }

    Skeleton::Bone::Bone(const Bone &bone)
    : mSkeleton(bone.mSkeleton)
    , mParent(bone.mParent)
    , mJointIndex(bone.mJointIndex)
    , mName(bone.mName)
    , mInverseBindPoseTransform(bone.mInverseBindPoseTransform)
    , mBindPoseTransform(bone.mBindPoseTransform)
    {
        if(bone.mChildBones.size() > 0)
        {
            throw od::Exception("Copying bone with set child links");
        }

        moveToBindPose();
    }

    void Skeleton::Bone::setInverseBindPoseTransform(const glm::mat4 &tform)
    {
        // turns out these aren't actually needed for animating a model. they are baked into the animations!
        //  still, i keep them here for future reference
        mInverseBindPoseTransform = tform;
        mBindPoseTransform = glm::inverse(tform);
    }

    size_t Skeleton::Bone::getChildBoneCount()
    {
        return mChildBones.size();
    }

    Skeleton::Bone &Skeleton::Bone::getChildBone(size_t index)
    {
        if(index >= mChildBones.size())
        {
            throw od::Exception("Child bone index out of bounds");
        }

        return *mChildBones[index];
    }

    Skeleton::Bone &Skeleton::Bone::addChildBone(int32_t jointIndex)
    {
        if(jointIndex < 0 || (size_t)jointIndex >= mSkeleton.mBones.size())
        {
            throw od::Exception("Child bone joint index passed to bone out of bounds");
        }

        Bone &childBone = mSkeleton.mBones[jointIndex];

        if(&childBone == this)
        {
            throw od::Exception("Tried to add bone to itself as a child");
        }

        childBone.mParent = this;
        mChildBones.push_back(&childBone);

        return childBone;
    }

    void Skeleton::Bone::moveToBindPose()
    {
        mCurrentMatrix = glm::mat4(1.0);
    }

    void Skeleton::Bone::move(const glm::mat4 &transform)
    {
        mCurrentMatrix = transform;
    }

    void Skeleton::Bone::_flattenRecursive(odRender::Rig &rig, const glm::mat4 &parentMatrix)
    {
        glm::mat4 chainMatrix = mCurrentMatrix * parentMatrix;

        rig.setBoneTransform(mJointIndex, chainMatrix);

        for(auto it = mChildBones.begin(); it != mChildBones.end(); ++it)
        {
            (*it)->_flattenRecursive(rig, chainMatrix);
        }
    }


    Skeleton::Skeleton(size_t boneCount)
    {
        mBones.reserve(boneCount);
        for(size_t i = 0; i < boneCount; ++i)
        {
            mBones.push_back(Bone(*this, i));
        }
    }

    Skeleton::Bone &Skeleton::addRootBone(int32_t jointIndex)
    {
        if(jointIndex < 0 || (size_t)jointIndex >= mBones.size())
        {
            throw od::Exception("Root bone joint index passed to skeleton out of bounds");
        }

        Bone &rootBone = mBones[jointIndex];
        rootBone.mParent = nullptr;
        mRootBones.push_back(&rootBone);

        return rootBone;
    }

    Skeleton::Bone &Skeleton::getBoneByJointIndex(int32_t jointIndex)
    {
        if(jointIndex < 0 || (size_t)jointIndex >= mBones.size())
        {
            throw od::Exception("Joint index passed to skeleton out of bounds");
        }

        return mBones[jointIndex];
    }

    void Skeleton::flatten(odRender::Rig &rig)
    {
        glm::mat4 eye(1.0);

        for(auto root : mRootBones)
        {
            root->_flattenRecursive(rig, eye);
        }
    }

    bool Skeleton::checkForLoops()
    {
        std::vector<bool> visited(mBones.size(), false);
        bool hasLoop = false;
        auto f = [&visited, &hasLoop](Bone &b)
        {
            if(visited[b.getJointIndex()])
            {
                hasLoop = true;
                return false;
            }

            visited[b.getJointIndex()] = true;
            return true;
        };

        traverse(f);

        return hasLoop;
    }
}
