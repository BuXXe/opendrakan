/*
 * Object.cpp
 *
 *  Created on: 8 Feb 2018
 *      Author: zal
 */

#include <odCore/LevelObject.h>

#include <algorithm>

#include <odCore/Client.h>
#include <odCore/Level.h>
#include <odCore/Layer.h>
#include <odCore/Exception.h>
#include <odCore/ObjectLightReceiver.h>

#include <odCore/anim/Skeleton.h>

#include <odCore/db/Model.h>
#include <odCore/db/ModelBounds.h>
#include <odCore/db/SkeletonBuilder.h>

#include <odCore/rfl/Class.h>
#include <odCore/rfl/ObjectBuilderProbe.h>

#include <odCore/render/Renderer.h>
#include <odCore/render/Handle.h>

#include <odCore/physics/PhysicsSystem.h>
#include <odCore/physics/Handles.h>

#include <odCore/state/StateManager.h>

namespace od
{

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

        // by comparing x and y of the coordinates relative to the cell center, we can check in which
        // triangle each point is. if they aren't in the same triangle, we crossed a boundary.

        // +----+  /\ Y/Z
        // |\0 /|
        // |3\/1|  > X
        // | /\ |
        // |/2 \|
        // +----+

        auto calcTriangle =  [](const glm::vec2 &p) -> int
        {
            // the two dividing lines are defined by y=x and y=-x. by turning the = into a relation,
            //  we can make the condition hold true for all points below the dividing lines. using
            //  clever combination, we can then determine in which triangle the point is, exactly
            bool in1or2 = p.x < p.y;
            bool in3or2 = p.y < -p.x;

            if(in1or2)
            {
                return in3or2 ? 2 : 1;

            }else
            {
                return in3or2 ? 3 : 0;
            }
        };

        if(calcTriangle(a) != calcTriangle(b))
        {
            return true;
        }

        return false;
    }


    LevelObject::LevelObject(Level &level, uint16_t recordIndex, ObjectRecordData &record, LevelObjectId id, std::shared_ptr<odDb::Class> dbClass)
    : mLevel(level)
    , mRecordIndex(recordIndex)
    , mId(record.getObjectId())
    , mClass(dbClass)
    , mLightingLayer(nullptr)
    , mPosition(record.getPosition())
    , mRotation(record.getRotation())
    , mScale(record.getScale())
    , mIsVisible(record.isVisible())
    , mState(LevelObjectState::NotLoaded)
    , mSpawnStrategy(SpawnStrategy::WhenInSight)
    , mAssociatedLayer(nullptr)
    , mAssociateWithCeiling(false)
    , mSpawnableClass(nullptr)
    , mRunObjectAi(true)
    , mEnableUpdate(false)
    {
        LayerId lightSourceLayerId = record.getLightSourceLayerId();
        if(lightSourceLayerId != 0)
        {
            mLightingLayer = mLevel.getLayerById(lightSourceLayerId);
            if(mLightingLayer == nullptr)
            {
                Logger::error() << "Object " << mId << " has invalid lighting layer ID " << lightSourceLayerId;
            }
        }

        // the record lists object indices, but we need a list of linked object IDs. translate them here
        mLinkedObjects.reserve(record.getLinkedObjectIndices().size());
        for(auto linkedIndex : record.getLinkedObjectIndices())
        {
            mLinkedObjects.push_back(level.getObjectIdForRecordIndex(linkedIndex));
        }
    }

    LevelObject::~LevelObject()
    {
        // make sure we perform the despawn cleanup in case we didnt despawn before getting deleted
        if(mState == LevelObjectState::Spawned)
        {
            Logger::warn() << "Level object deleted while still spawned";
            despawned();
        }

        Logger::debug() << "Object " << getObjectId() << " destroyed";

        setEnableUpdate(false);
    }

    void LevelObject::setPosition(const glm::vec3 &v)
    {
        _applyTranslation(v);
        mLevel.getEngine().getStateManager().objectTranslated(*this, v);
    }

    void LevelObject::setRotation(const glm::quat &q)
    {
        _applyRotation(q);
        mLevel.getEngine().getStateManager().objectRotated(*this, q);
    }

    void LevelObject::setScale(const glm::vec3 &s)
    {
        _applyScale(s);
        mLevel.getEngine().getStateManager().objectScaled(*this, s);
    }

    void LevelObject::setVisible(bool v)
    {
        _applyVisibility(v);

        mLevel.getEngine().getStateManager().objectVisibilityChanged(*this, v);
    }

    void LevelObject::applyStates(const ObjectStates &states)
    {
        if(states.position.hasValue()) _applyTranslation(states.position.get());
        if(states.rotation.hasValue()) _applyRotation(states.rotation.get());
        if(states.scale.hasValue()) _applyScale(states.scale.get());
        if(states.visibility.hasValue()) _applyVisibility(states.visibility.get());
    }

    void LevelObject::extraStateDirty()
    {
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

        mState = LevelObjectState::Loaded;

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onDespawned();
        }
    }

    void LevelObject::messageReceived(LevelObject &sender, od::Message message)
    {
        Logger::verbose() << "Object " << getObjectId() << " received message '" << message << "' from " << sender.getObjectId();

        if(mRunObjectAi && mSpawnableClass != nullptr)
        {
            mSpawnableClass->onMessageReceived(sender, message);
        }
    }

    void LevelObject::setEnableUpdate(bool enable)
    {
        mEnableUpdate = enable;
    }

    void LevelObject::update(float relTime)
    {
        if(mState != LevelObjectState::Spawned)
        {
            return;
        }

        if(mRunObjectAi && mEnableUpdate && mSpawnableClass != nullptr)
        {
            mSpawnableClass->onUpdate(relTime);
        }
    }

    void LevelObject::postUpdate(float relTime)
    {
        if(mRunObjectAi && mEnableUpdate && mSpawnableClass != nullptr)
        {
            mSpawnableClass->onPostUpdate(relTime);
        }
    }

    void LevelObject::setAssociatedLayer(od::Layer *newLayer)
    {
        od::Layer *oldLayer = mAssociatedLayer;
        mAssociatedLayer = newLayer;

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onLayerChanged(oldLayer, newLayer);
        }
    }

    odAnim::Skeleton *LevelObject::getOrCreateSkeleton()
    {
        if(mSkeleton == nullptr && mClass != nullptr && mClass->hasModel() && mClass->getModel()->hasSkeleton())
        {
            odDb::SkeletonBuilder *sb = mClass->getModel()->getSkeletonBuilder();
            mSkeleton = std::make_unique<odAnim::Skeleton>(sb->getJointCount());
            sb->build(*mSkeleton);
        }

        return mSkeleton.get();
    }

    void LevelObject::attachTo(LevelObject *target, bool ignoreTranslation, bool ignoreRotation, bool ignoreScale)
    {
        OD_UNIMPLEMENTED();
    }

    void LevelObject::attachToChannel(LevelObject *target, size_t channelIndex, bool clearOffset)
    {
        OD_UNIMPLEMENTED();
    }

    void LevelObject::detach()
    {
        OD_UNIMPLEMENTED();
    }

    void LevelObject::messageAllLinkedObjects(od::Message message)
    {
        for(auto linkedId : mLinkedObjects)
        {
            od::LevelObject *obj = mLevel.getLevelObjectById(linkedId);
            if(obj != nullptr)
            {
                obj->messageReceived(*this, message);
            }
        }
    }

    void LevelObject::requestDestruction()
    {
        mLevel.addToDestructionQueue(getObjectId());
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

    void LevelObject::setRflClassInstance(std::unique_ptr<odRfl::ClassBase> i)
    {
        mRflClassInstance = std::move(i);

        if(mRflClassInstance != nullptr)
        {
            mSpawnableClass = mRflClassInstance->asSpawnableClass();
            if(mSpawnableClass != nullptr)
            {
                mSpawnableClass->setLevelObject(*this);

            }else
            {
                Logger::warn() << "Level object has RFL class that is not spawnable. This object will probably not do much...";
            }

            if(mClass != nullptr)
            {
                mClass->fillFields(mRflClassInstance->getFields());
            }

            auto &fieldLoader = mLevel.getObjectRecord(mRecordIndex).getFieldLoader();
            fieldLoader.reset();
            mRflClassInstance->getFields().probeFields(fieldLoader);

            mRflClassInstance->onLoaded();
        }
    }

    void LevelObject::setupRenderingAndPhysics(ObjectRenderMode renderMode, ObjectPhysicsMode physicsMode)
    {
        mRenderMode = renderMode;
        mPhysicsMode = physicsMode;

        // create physics first because lighting might need the handle
        if(physicsMode != ObjectPhysicsMode::NO_PHYSICS)
        {
            mPhysicsHandle = getLevel().getEngine().getPhysicsSystem().createObjectHandle(*this, false);
        }

        if(renderMode != ObjectRenderMode::NOT_RENDERED && mClass != nullptr && mClass->hasModel())
        {
            if(getLevel().getEngine().isServer())
            {
                throw od::Exception("Can't enable rendering on servers");
            }

            auto &client = getLevel().getEngine().getClient();
            auto &renderer = client.getRenderer();
            auto dbModel = mClass->getModel();
            auto renderModel = renderer.getOrCreateModelFromDb(dbModel);

            mRenderHandle = renderer.createHandle(odRender::RenderSpace::LEVEL);
            mRenderHandle->setPosition(this->getPosition());
            mRenderHandle->setOrientation(this->getRotation());
            mRenderHandle->setScale(this->getScale());
            mRenderHandle->setModel(renderModel);

            if(renderMode != ObjectRenderMode::NO_LIGHTING)
            {
                od::Layer *lightSourceLayer = this->getLightSourceLayer();
                if(lightSourceLayer == nullptr) lightSourceLayer = this->getAssociatedLayer();
                if(lightSourceLayer != nullptr)
                {
                    mRenderHandle->setGlobalLight(lightSourceLayer->getLightDirection(), lightSourceLayer->getLightColor(), lightSourceLayer->getAmbientColor());
                }

                if(mPhysicsHandle != nullptr)
                {
                    mLightReceiver = std::make_unique<od::ObjectLightReceiver>(client.getPhysicsSystem(), mPhysicsHandle, mRenderHandle);
                    mLightReceiver->updateAffectingLights();

                }else
                {
                    Logger::warn() << "Object " << mId << " without physics but with lighting enabled will not receive light from point lights";
                }
            }
        }
    }

    void LevelObject::setRunState(bool run)
    {
        mRunObjectAi = run;
    }
    void LevelObject::_applyTranslation(const glm::vec3 &p)
    {
        auto prevPos = getPosition();
        mPosition = p;

        if(_hasCrossedTriangle(prevPos, mPosition))
        {
            updateAssociatedLayer(true);
        }

        if(mRenderHandle != nullptr)
        {
            mRenderHandle->setPosition(mPosition);
        }

        if(mPhysicsHandle != nullptr)
        {
            mPhysicsHandle->setPosition(mPosition);
        }

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onTranslated(prevPos, mPosition);
            mSpawnableClass->onTransformChanged();
        }
    }

    void LevelObject::_applyRotation(const glm::quat &r)
    {
        glm::quat prevRot = mRotation;
        mRotation = r;

        if(mRenderHandle != nullptr)
        {
            mRenderHandle->setOrientation(mRotation);
        }

        if(mPhysicsHandle != nullptr)
        {
            mPhysicsHandle->setOrientation(mRotation);
        }

        if(mRunObjectAi && mSpawnableClass != nullptr)
        {
            mSpawnableClass->onRotated(prevRot, mRotation);
            mSpawnableClass->onTransformChanged();
        }
    }

    void LevelObject::_applyScale(const glm::vec3 &s)
    {
        glm::vec3 prevScale = mScale;
        mScale = s;

        if(mRenderHandle != nullptr)
        {
            mRenderHandle->setScale(mScale);
        }

        if(mPhysicsHandle != nullptr)
        {
            mPhysicsHandle->setScale(mScale);
        }

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onScaled(prevScale, mScale);
            mSpawnableClass->onTransformChanged();
        }
    }

    void LevelObject::_applyVisibility(bool v)
    {
        Logger::verbose() << "Object " << getObjectId() << " made " << (v ? "visible" : "invisible");

        mIsVisible = v;

        if(mRenderHandle != nullptr)
        {
            mRenderHandle->setVisible(mIsVisible);
        }

        if(mSpawnableClass != nullptr)
        {
            mSpawnableClass->onVisibilityChanged(v);
        }
    }

}
