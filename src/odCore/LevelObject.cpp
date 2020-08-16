/*
 * Object.cpp
 *
 *  Created on: 8 Feb 2018
 *      Author: zal
 */

#include <odCore/LevelObject.h>

#include <algorithm>

#include <odCore/Level.h>
#include <odCore/Layer.h>
#include <odCore/Exception.h>
#include <odCore/OdDefines.h>

#include <odCore/anim/Skeleton.h>

#include <odCore/db/Model.h>
#include <odCore/db/ModelBounds.h>
#include <odCore/db/SkeletonBuilder.h>

#include <odCore/rfl/Class.h>
#include <odCore/rfl/ObjectBuilderProbe.h>

#include <odCore/physics/PhysicsSystem.h>
#include <odCore/physics/Handles.h>

#define OD_OBJECT_FLAG_VISIBLE 0x001
#define OD_OBJECT_FLAG_SCALED  0x100

namespace od
{


    class NonRecordingObjectStateHandle final : public odState::ObjectStateHandle
    {
    public:

        virtual void setPosition(const glm::vec3 &v) override
        {

        }

        virtual void setRotation(const glm::quat &q) override
        {

        }

        virtual void setScale(const glm::vec3 &scale) override
        {

        }

        virtual void setVisible(bool v) override
        {

        }

    };



    /**
     * @brief Checks whether a translation from a to b crosses a triangle on the unit layer grid.
     *
     * This is used to limit the times the layer association is updated, since that only has to happen when
     * an object moves across a triangle in the grid (we have to check both triangles since we don't know how
     * the triangles of our layer are oriented. we could store this based on the last association, but i doubt that
     * will have much of an impact).
     */
    static bool _hasCrossedTriangle(const glm::vec3 &p_a, const glm::vec3 &p_b)
    {
        // we translate the coordinates: the center of the grid cell that contains a becomes (0,0)
        glm::vec2 ref = glm::floor(glm::vec2(p_a.x, p_a.z)) + glm::vec2(0.5, 0.5);
        glm::vec2 a = glm::vec2(p_a.x, p_a.z) - ref;
        glm::vec2 b = glm::vec2(p_b.x, p_b.z) - ref;

        // if b is not even in the same cell anymore, we obviously crossed a triangle
        if(b.x < -0.5 || b.x > 0.5 || b.y < -0.5 || b.y > 0.5)
        {
            return true;
        }

        // using the signs of the coordinates relative to the cell center, we can check in which
        // triangle each point is. if they aren't in the same triangle, we crossed a boundary.

        // +----+  /\ Y/Z
        // |\0 /|
        // |3\/1|  > X
        // | /\ |
        // |/2 \|
        // +----+

        auto calcTriangle =  [](const glm::vec2 &p) -> int
        {
            if(p.y >= p.x)
            {
                // p is in 0 or 2
                return (p.y > 0) ? 0 : 2;

            }else
            {
                // a is in 3 or 1
                return (p.x > 0) ? 1 : 3;
            }
        };

        if(calcTriangle(a) != calcTriangle(b))
        {
            return true;
        }

        return false;
    }


    LevelObject::LevelObject(Level &level)
    : mLevel(level)
    , mId(0)
    , mLightingLayerId(0)
    , mLightingLayer(nullptr)
    , mFlags(0)
    , mInitialEventCount(0)
    , mIsVisible(true)
    , mState(LevelObjectState::NotLoaded)
    , mObjectType(LevelObjectType::Normal)
    , mSpawnStrategy(SpawnStrategy::WhenInSight)
    , mAttachmentTarget(nullptr)
    , mIgnoreAttachmentTranslation(false)
    , mIgnoreAttachmentRotation(false)
    , mIgnoreAttachmentScale(false)
    , mAssociatedLayer(nullptr)
    , mAssociateWithCeiling(false)
    , mSpawnableClass(nullptr)
    {
    }

    LevelObject::~LevelObject()
    {
        // make sure we perform the despawn cleanup in case we didnt despawn before getting deleted
        if(mState == LevelObjectState::Spawned)
        {
            Logger::warn() << "Level object deleted while still spawned";

            despawned();
        }
    }

    void LevelObject::loadFromRecord(DataReader dr)
    {
        uint16_t xRot;
        uint16_t yRot;
        uint16_t zRot;
        uint16_t linkCount;

        dr >> mId
           >> mClassRef
           >> mLightingLayerId
           >> mInitialPosition
           >> mFlags
           >> mInitialEventCount
           >> linkCount;

        if(mLightingLayerId != 0)
        {
            mLightingLayer = mLevel.getLayerById(mLightingLayerId);
            if(mLightingLayer == nullptr)
            {
                Logger::error() << "Object " << mId << " has invalid lighting layer ID " << mLightingLayerId;
            }
        }

        // TODO: finally write a deserializer for vectors of things!
        mLinkedObjectIndices.resize(linkCount);
        for(size_t i = 0; i < linkCount; ++i)
        {
            dr >> mLinkedObjectIndices[i];
        }

        dr >> xRot
           >> yRot
           >> zRot;

        if(mFlags & OD_OBJECT_FLAG_SCALED)
        {
            dr >> mInitialScale;

        }else
        {
            mInitialScale = glm::vec3(1, 1, 1);
        }

        odRfl::ObjectBuilderProbe builder;
        builder.readFieldRecord(dr);

        // all fields loaded. now create class instance TODO: maybe move this to a second method so we don't invoke class behavior just by loading the fields?
        mClass = mLevel.getAssetByRef<odDb::Class>(mClassRef);
        auto instance = mClass->makeInstance(mLevel.getEngine());
        if(instance != nullptr)
        {
            instance->getFields().probeFields(builder);
            this->replaceRflClassInstance(std::move(instance));

        }else
        {
            // atm, this really crowds out the console log. to keep things readable, we'll only log once for now
            // Logger::warn() << "Level object has umimplemented RFL class";
            static bool warnOnce = false;
            if(warnOnce)
            {
                Logger::warn() << "Level object has umimplemented RFL class (not gonna warn again because of this)";
                warnOnce = true;
            }
        }

        mInitialPosition *= OD_WORLD_SCALE; // correct editor scaling

        glm::vec3 eulerAngles(glm::radians((float)xRot), glm::radians((float)yRot-90), glm::radians((float)zRot)); // -90 deg. determined to be correct through experiment
        mInitialRotation = glm::quat(eulerAngles);

        mPosition = mInitialPosition;
        mRotation = mInitialRotation;
        mScale = mInitialScale;

        mIsVisible = mFlags & OD_OBJECT_FLAG_VISIBLE;

        if(mRflClassInstance != nullptr)
        {
            mRflClassInstance->onLoaded(); // do this last! it'd be better to pass a fully set up level object here
        }
    }

    odState::ObjectStateHandle &LevelObject::getNonRecordingStateHandle()
    {
        throw UnsupportedException("not yet implemented");
    }

    void LevelObject::spawned()
    {
        Logger::debug() << "Object " << getObjectId() << " spawned";

        // if we haven't got an associated layer yet, search for it now.
        // TODO: add flag tracking this. might be possible we searched once, but found there is no layer to associate with
        if(mAssociatedLayer == nullptr)
        {
            updateAssociatedLayer(false);
        }

        mState = LevelObjectState::Spawned;

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onSpawned();
        }
    }

    void LevelObject::despawned()
    {
        Logger::debug() << "Object " << getObjectId() << " despawned";

        // detach this from any object it may be attached to, and detach all objects attached to this
        detach();
        _detachAllAttachedObjects();

        mState = LevelObjectState::Loaded;

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onDespawned();
        }
    }

    void LevelObject::destroyed()
    {
        Logger::verbose() << "Object " << getObjectId() << " destroyed";

        mState = LevelObjectState::Destroyed;

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onDestroyed();
        }
    }

    void LevelObject::messageReceived(LevelObject &sender, od::Message message)
    {
        if(mState == LevelObjectState::Destroyed)
        {
            return;
        }

        Logger::verbose() << "Object " << getObjectId() << " received message '" << message << "' from " << sender.getObjectId();

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onMessageReceived(sender, message);
        }
    }

    void LevelObject::setEnableUpdate(bool enable)
    {
        if(enable)
        {
            mLevel.addToUpdateQueue(*this);

        }else
        {
            mLevel.removeFromUpdateQueue(*this);
        }
    }

    void LevelObject::update(float relTime)
    {
        if(mState != LevelObjectState::Spawned)
        {
            return;
        }

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onUpdate(relTime);
        }
    }

    void LevelObject::postUpdate()
    {
    }

    void LevelObject::setPosition(const glm::vec3 &v)
    {
        glm::vec3 prevPos = mPosition;
        mPosition = v;

        if(_hasCrossedTriangle(prevPos, v))
        {
            updateAssociatedLayer(true);
        }

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onTranslated(prevPos, v);
        }

        _onTransformChanged(this);
    }

    void LevelObject::setRotation(const glm::quat &q)
    {
        glm::quat prevRot = mRotation;
        mRotation = q;

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onRotated(prevRot, q);
        }

        _onTransformChanged(this);
    }

    void LevelObject::setScale(const glm::vec3 &scale)
    {
        glm::vec3 prevScale = mScale;
        mScale = scale;

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onScaled(prevScale, scale);
        }

        _onTransformChanged(this);
    }

    void LevelObject::setVisible(bool v)
    {
        Logger::verbose() << "Object " << getObjectId() << " made " << (v ? "visible" : "invisible");

        mIsVisible = v;

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onVisibilityChanged(v);
        }
    }

    void LevelObject::setAssociatedLayer(od::Layer *newLayer)
    {
        od::Layer *oldLayer = mAssociatedLayer;
        mAssociatedLayer = newLayer;

        if(/*callChangedHook && */mSpawnableClass != nullptr)
        {
            mSpawnableClass->onLayerChanged(oldLayer, newLayer);
        }
    }

    void LevelObject::setObjectType(LevelObjectType type)
    {
        mObjectType = type;

        bool objectVisible;
        switch(type)
        {
        case LevelObjectType::Normal:
            objectVisible = true;
            break;

        case LevelObjectType::Light:
        case LevelObjectType::Detector:
        default:
            objectVisible = false;
            break;
        }

        (void)objectVisible;
    }

    odAnim::Skeleton *LevelObject::getOrCreateSkeleton()
    {
        if(mSkeleton == nullptr && mClass->hasModel() && mClass->getModel()->hasSkeleton())
        {
            odDb::SkeletonBuilder *sb = mClass->getModel()->getSkeletonBuilder();
            mSkeleton = std::make_unique<odAnim::Skeleton>(sb->getJointCount());
            sb->build(*mSkeleton);
        }

        return mSkeleton.get();
    }

    void LevelObject::attachTo(LevelObject *target, bool ignoreTranslation, bool ignoreRotation, bool ignoreScale)
    {
        if(target == nullptr || mAttachmentTarget != nullptr)
        {
            detach();

            if(target == nullptr)
            {
                return;
            }
        }

        if(!ignoreRotation)
        {
            throw UnsupportedException("Attachment with rotation is unimplemented");
        }

        mAttachmentTarget = target;
        target->mAttachedObjects.push_back(this);

        mIgnoreAttachmentTranslation = ignoreTranslation;
        mIgnoreAttachmentRotation = ignoreRotation;
        mIgnoreAttachmentScale = ignoreScale;

        mAttachmentTranslationOffset = getPosition() - target->getPosition();
        mAttachmentRotationOffset = glm::inverse(getRotation()) * target->getRotation();
        mAttachmentScaleRatio = getScale() / target->getScale();
    }

    void LevelObject::attachToChannel(LevelObject *target, size_t channelIndex, bool clearOffset)
    {
        // TODO: implement

        throw UnsupportedException("Attaching to channels not yet implemented.");
    }

    void LevelObject::detach()
    {
        if(mAttachmentTarget == nullptr)
        {
            return;
        }

        auto it = std::find(mAttachmentTarget->mAttachedObjects.begin(), mAttachmentTarget->mAttachedObjects.end(), this);
        if(it != mAttachmentTarget->mAttachedObjects.end())
        {
            mAttachmentTarget->mAttachedObjects.erase(it);

        }else
        {
            Logger::warn() << "Inconsistency when detaching object: Attachment target didn't have attached object in it's list of attached objects";
        }

        mAttachmentTarget = nullptr;
    }

    void LevelObject::messageAllLinkedObjects(od::Message message)
    {
        for(auto linkedIndex : mLinkedObjectIndices)
        {
            od::LevelObject *obj = mLevel.getLevelObjectByIndex(linkedIndex);
            if(obj != nullptr)
            {
                obj->messageReceived(*this, message);
            }
        }
    }

    void LevelObject::requestDestruction()
    {
        mLevel.requestLevelObjectDestruction(this);
    }

    AxisAlignedBoundingBox LevelObject::getBoundingBox()
    {
        if(mClass == nullptr || !mClass->hasModel())
        {
            return AxisAlignedBoundingBox(mPosition, mPosition);
        }

        auto &modelBB = mClass->getModel()->getCalculatedBoundingBox();

        glm::vec3 min = (modelBB.min() * mScale) * mRotation + mPosition;
        glm::vec3 max = (modelBB.max() * mScale) * mRotation + mPosition;

        return AxisAlignedBoundingBox(min, max);
    }

    BoundingSphere LevelObject::getBoundingSphere()
    {
        if(mClass == nullptr || !mClass->hasModel())
        {
            return BoundingSphere(mPosition, 0);
        }

        float calcRadius = mClass->getModel()->getCalculatedBoundingSphere().radius();
        float maxScale = std::max(std::max(mScale.x, mScale.y), mScale.z);

        return BoundingSphere(mPosition, calcRadius*maxScale);
    }

    void LevelObject::updateAssociatedLayer(bool callChangedHook)
    {
        odPhysics::PhysicsSystem &ps = mLevel.getPhysicsSystem();

        // a slight upwards offset fixes many association issues with objects whose origin is exactly on the ground
        glm::vec3 rayStart = mPosition + (mAssociateWithCeiling ? glm::vec3(0, -0.1, 0) : glm::vec3(0, 0.1, 0));

        float heightOffset =  mLevel.getVerticalExtent() * (mAssociateWithCeiling ? 1 : -1);
        glm::vec3 rayEnd = mPosition + glm::vec3(0, heightOffset, 0);

        odPhysics::RayTestResult hitResult;
        ps.rayTestClosest(rayStart, rayEnd, odPhysics::PhysicsTypeMasks::Layer, nullptr, hitResult);

        od::Layer *oldLayer = mAssociatedLayer;
        od::Layer *newLayer = (hitResult.handle == nullptr) ? nullptr : &hitResult.handle->asLayerHandle()->getLayer();

        if(oldLayer != newLayer)
        {
            setAssociatedLayer(newLayer);
        }
    }

    void LevelObject::replaceRflClassInstance(std::unique_ptr<odRfl::ClassBase> i)
    {
        // TODO: maybe signal the class instance that's being replaced? like, add onInstanceReplaced() to spawnable
        mRflClassInstance = std::move(i);
        mSpawnableClass = mRflClassInstance->asSpawnableClass();
        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->setLevelObject(*this);

        }else
        {
            Logger::warn() << "Level object has RFL class that is not spawnable. This object will probably not do much...";
        }
    }

    void LevelObject::_onTransformChanged(LevelObject *transformChangeSource)
    {
        for(auto it = mAttachedObjects.begin(); it != mAttachedObjects.end(); ++it)
        {
            (*it)->_attachmentTargetsTransformUpdated(this);
        }

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onTransformChanged();
        }
    }

    void LevelObject::_attachmentTargetsTransformUpdated(LevelObject *transformChangeSource)
    {
        if(transformChangeSource == this)
        {
            Logger::warn() << "Circular attachment detected! Will try to fix this by detaching one object in the loop";
            detach();
            return;
        }

        if(mAttachmentTarget == nullptr)
        {
            // uhmm.... wat?
            Logger::error() << "Attachment inconsistency: Level object that was not attached to anything was asked to update to attachment target position";
            return;
        }

        // note: don't call this->setPosition etc. here. it would call _onTransformChanged using this as argument, but we want
        //  to pass along transformChangeSource so we can detect circular attachments

        if(!mIgnoreAttachmentTranslation)
        {
            mPosition = mAttachmentTarget->getPosition() + mAttachmentTranslationOffset;
        }

        if(!mIgnoreAttachmentRotation)
        {
            // TODO: implement
        }

        if(!mIgnoreAttachmentScale)
        {
            mScale = mAttachmentTarget->getScale() * mAttachmentScaleRatio;
        }

        _onTransformChanged(transformChangeSource);
    }

    void LevelObject::_detachAllAttachedObjects()
    {
        // note: we don't call detach() on all attached objects here, as that would cause every single one of them
        //  to search itself in mAttachedObjects, unecessarily making this O(n^2)
        for(auto it = mAttachedObjects.begin(); it != mAttachedObjects.end(); ++it)
        {
            (*it)->mAttachmentTarget = nullptr;
        }

        mAttachedObjects.clear();
    }

}
