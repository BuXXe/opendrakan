/*
 * Object.h
 *
 *  Created on: 8 Feb 2018
 *      Author: zal
 */

#ifndef OBJECT_H_
#define OBJECT_H_

#include <memory>
#include <array>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <odCore/BoundingBox.h>
#include <odCore/BoundingSphere.h>
#include <odCore/IdTypes.h>
#include <odCore/Message.h>
#include <odCore/ObjectTransform.h>

#include <odCore/db/Class.h>

namespace odAnim
{
    class Skeleton;
}

namespace odRender
{
    class Handle;
}

namespace odPhysics
{
    class ObjectHandle;
}

namespace od
{
    class ObjectLightReceiver;
    class Level;
    class Layer;

    enum class LevelObjectState
    {
        NotLoaded,
        Loaded,
        Spawned,
        Destroyed
    };

    enum class LevelObjectType
    {
        Normal,
        Light,
        Detector
    };

    enum class SpawnStrategy
    {
        Never,
        WhenInSight,
        Always
    };


    class LevelObject
    {
    public:

        LevelObject(Level &level);
        virtual ~LevelObject();

        inline LevelObjectId getObjectId() const { return mId; }
        inline std::shared_ptr<odDb::Class> getClass() { return mClass; }
        inline odRfl::ClassBase *getClassInstance() { return mRflClassInstance.get(); }
        inline Level &getLevel() { return mLevel; }
        inline glm::vec3 getPosition() const { return mPosition; }
        inline glm::vec3 getScale() const { return mScale; }
        inline glm::quat getRotation() const { return mRotation; }
        inline LevelObjectState getState() const { return mState; }
        inline LevelObjectType getObjectType() const { return mObjectType; }
        inline void setSpawnStrategy(SpawnStrategy s) { mSpawnStrategy = s; }
        inline SpawnStrategy getSpawnStrategy() const { return mSpawnStrategy; }
        inline const std::vector<LevelObjectId> &getLinkedObjects() const { return mLinkedObjects; }
        inline Layer *getLightSourceLayer() { return mLightingLayer; }
        inline bool isVisible() const { return mIsVisible; }
        inline bool isScaled() const { return (mScale != glm::vec3(1,1,1)); }
        inline void setAssociateWithCeiling(bool b) { mAssociateWithCeiling = b; }
        inline Layer *getAssociatedLayer() const { return mAssociatedLayer; } ///< @return The layer this object is associated with, or nullptr if none

        void loadFromRecord(DataReader dr);
        void translateLinkIndices(const std::array<LevelObjectId, 0x10000> &idMap);

        void spawned();
        void despawned();
        void destroyed();
        void messageReceived(LevelObject &sender, od::Message message);

        void transform(const ObjectTransform &tf);
        void setVisible(bool v);

        // convenience methods. these just call transform() with a specially constructed ObjectTransform
        inline void setPosition(const glm::vec3 &v)
        {
            ObjectTransform tf;
            tf.setPosition(v);
            transform(tf);
        }

        inline void setRotation(const glm::quat &q)
        {
            ObjectTransform tf;
            tf.setRotation(q);
            transform(tf);
        }

        inline void setScale(const glm::vec3 &s)
        {
            ObjectTransform tf;
            tf.setScale(s);
            transform(tf);
        }

        void customStateDirty() {}
        void setCustomStateModes(bool networked, bool lerped) {}

        /**
         * @brief Enables or disables updates for this object.
         *
         * Changing this in the update hook will not prevent the postUpdate hook to be called. The change
         * will only become effective with the next update.
         */
        void setEnableUpdate(bool enable);

        /**
         * @brief Called each tick during the update stage.
         * @param relTime  The time passed since the last update, in seconds.
         */
        void update(float relTime);

        /**
         * @brief Called after everything in the level has been updated and a snapshot is about to occur.
         *
         * This is where an object can perform updates that only need to reflect the final level state for this tick.
         * These updates should not affect other objects.
         *
         * One example for what might happen here is updating the object's associated layer, which only needs to consider
         * the final position assigned during this tick.
         */
        void postUpdate();

        void setAssociatedLayer(od::Layer *l);

        void setObjectType(LevelObjectType type);

        /**
         * @brief Returns the skeleton for this object or builds it if that has not yet been done.
         *
         * This may return nullptr if this object has no model or the object's model has no skeleton.
         */
        odAnim::Skeleton *getOrCreateSkeleton();

        /**
         * @brief Attaches this object to target object.
         *
         * Attaches this LevelObject to the LevelObject \c target. Any transformation applied to \c target will also be applied to
         * \c this. This only happens in a relative context. For instance, any offset between this and \c target will be maintained
         * unless \c ignoreTranslation is true and the scale ration between these two object will be maintained unless
         * \c ignoreScale is true.
         *
         * If \c ignoreTranslation is true, translation changes applied to \c target will not be transferred to this object.
         * If \c ignoreRotation is true, rotation  changesapplied to \c target will not be transferred to this object.
         * If \c ignoreScale is true, scaling changes applied to \c target will not be transferred to this object.
         *
         * If \c ignoreTranslation, \c ignoreRotation and \c ignoreScale are all true, this method will throw.
         *
         * Calling this with \c target as \c nullptr is equivalent to calling \c detach().
         *
         * If this is called on a LevelObject that is already attached to something, it will be detached first.
         *
         * If the attachment target despawns, this object will automatically be detached.
         */
        void attachTo(LevelObject *target, bool ignoreTranslation, bool ignoreRotation, bool ignoreScale);

        /**
         * @brief Attaches this object to bone/channel of target character object.
         *
         * Attaches this LevelObject to the channel with index \c channelIndex in LevelObject \c target.
         * Any transformation applied to the respective channel in \c target will also be applied to
         * \c this. Any rotations transferred from the channel to this will pivot relative to the channel's origin.
         * If \c target has no skeleton or no channel with the given index, this method will throw.
         *
         * If \c clearOffset is true, \c this will be moved to target channel prior to attachment so their transformation will be kept equal
         * for the duration of the attachment.
         *
         * If this is called on a LevelObject that is already attached to something, it will be detached first.
         */
        void attachToChannel(LevelObject *target, size_t channelIndex, bool clearOffset = false);

        /**
         * @brief Detaches this object from any objects. Will do nothing if this object was not attached to anything.
         */
        void detach();

        void messageAllLinkedObjects(od::Message message);
        void requestDestruction();

        /**
         * @brief Calculates a axis-aligned bounding box for this object from it's model, taking position, rotation and scale into account.
         */
        AxisAlignedBoundingBox getBoundingBox();

        /**
         * @brief Calculates a bounding sphere for this object from it's model, taking position and scale into account.
         *
         * Note that rotation does not affect bounding spheres.
         */
        BoundingSphere getBoundingSphere();

        void updateAssociatedLayer(bool callChangedHook = true);

        /**
         * @brief Replaces the current RFL class instance (if any).
         *
         * Use this carefully. If the class that's being replaced is not a Dummy
         * or null, the class must handle it's being replaced explicitly. No
         * despawned, destroyed etc. hooks will be called because the object is
         * not despawned or destroid during this process!
         */
        void replaceRflClassInstance(std::unique_ptr<odRfl::ClassBase> i);



    private:

        void _applyTransform(const ObjectTransform &tf, LevelObject *transformChangeSource);
        void _applyVisibility(bool v);
        void _attachmentTargetsTransformUpdated(LevelObject *transformChangeSource); // pass along source so we can detect circular attachments
        void _detachAllAttachedObjects();

        Level &mLevel;

        // loaded from the object record:
        LevelObjectId mId;
        odDb::AssetRef mClassRef;
        std::shared_ptr<odDb::Class> mClass;
        uint32_t mLightingLayerId;
        Layer *mLightingLayer;
        uint32_t mFlags;
        uint16_t mInitialEventCount;
        std::vector<LevelObjectId> mLinkedObjects; // this is sorta abused, since during load it stores the indices instead. those are later translated
        glm::vec3 mInitialPosition;
        glm::quat mInitialRotation;
        glm::vec3 mInitialScale;

        // state:
        glm::vec3 mPosition;
        glm::quat mRotation;
        glm::vec3 mScale;
        bool mIsVisible;

        LevelObjectState mState;
        LevelObjectType mObjectType;
        SpawnStrategy mSpawnStrategy;

        od::LevelObject *mAttachmentTarget;
        std::vector<od::LevelObject*> mAttachedObjects;
        glm::vec3 mAttachmentTranslationOffset;
        glm::quat mAttachmentRotationOffset;
        glm::vec3 mAttachmentScaleRatio;
        bool mIgnoreAttachmentTranslation;
        bool mIgnoreAttachmentRotation;
        bool mIgnoreAttachmentScale;

        Layer *mAssociatedLayer;
        bool mAssociateWithCeiling;

        // created on demand:
        std::unique_ptr<odAnim::Skeleton> mSkeleton;

        std::unique_ptr<odRfl::ClassBase> mRflClassInstance;
        odRfl::SpawnableClass *mSpawnableClass; // downcast version of mRflClassInstance, so we don't have to cast for every call to Spawnable methods

    };

}



#endif /* OBJECT_H_ */
