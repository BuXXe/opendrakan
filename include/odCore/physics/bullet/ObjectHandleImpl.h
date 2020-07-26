/*
 * ObjectHandleImpl.h
 *
 *  Created on: Feb 11, 2019
 *      Author: zal
 */

#ifndef INCLUDE_ODCORE_PHYSICS_BULLET_OBJECTHANDLEIMPL_H_
#define INCLUDE_ODCORE_PHYSICS_BULLET_OBJECTHANDLEIMPL_H_

#include <memory>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

#include <odCore/physics/Handles.h>

namespace od
{
    class LevelObject;
}

namespace odBulletPhysics
{
    class BulletPhysicsSystem;
    class ModelShape;

    class ObjectHandle final : public odPhysics::ObjectHandle
    {
    public:

        ObjectHandle(BulletPhysicsSystem &ps, od::LevelObject &obj, btCollisionWorld *collisionWorld, bool isDetector);
        virtual ~ObjectHandle();

        inline btCollisionObject *getBulletObject() { return mCollisionObject.get(); }

        virtual void setEnableCollision(bool b) override;

        virtual void setPosition(const glm::vec3 &p) override;
        virtual void setOrientation(const glm::quat &q) override;
        virtual void setScale(const glm::vec3 &s) override;

        virtual od::LevelObject &getLevelObject() override;


    private:

        od::LevelObject &mLevelObject;
        btCollisionWorld *mCollisionWorld;

        std::shared_ptr<ModelShape> mModelShape;

        std::unique_ptr<btCollisionShape> mUniqueShape;
        std::unique_ptr<btCollisionObject> mCollisionObject;
    };

}


#endif /* INCLUDE_ODCORE_PHYSICS_BULLET_OBJECTHANDLEIMPL_H_ */
