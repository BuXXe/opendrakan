/*
 * LightHandleImpl.h
 *
 *  Created on: 17 Feb 2019
 *      Author: zal
 */

#ifndef INCLUDE_ODCORE_PHYSICS_BULLET_LIGHTHANDLEIMPL_H_
#define INCLUDE_ODCORE_PHYSICS_BULLET_LIGHTHANDLEIMPL_H_

#include <memory>

#include <glm/vec3.hpp>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>

#include <odCore/physics/Handles.h>

namespace od
{
    class Light;
}

namespace odBulletPhysics
{

    class LightHandle final : public odPhysics::LightHandle
    {
    public:

        LightHandle(std::shared_ptr<od::Light> light, btCollisionWorld *collisionWorld);
        virtual ~LightHandle();

        inline btCollisionObject *getBulletObject() { return mCollisionObject.get(); }

        virtual void setRadius(float radius) override;
        virtual void setPosition(const glm::vec3 &pos) override;

        virtual std::shared_ptr<od::Light> getLight() override;


    private:

        std::weak_ptr<od::Light> mLight;
        btCollisionWorld *mCollisionWorld;

        std::unique_ptr<btSphereShape> mShape;
        std::unique_ptr<btCollisionObject> mCollisionObject;

    };

}

#endif /* INCLUDE_ODCORE_PHYSICS_BULLET_LIGHTHANDLEIMPL_H_ */
