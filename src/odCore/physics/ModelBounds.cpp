/*
 * ModelBounds.cpp
 *
 *  Created on: 13 Apr 2018
 *      Author: zal
 */

#include <odCore/physics/ModelBounds.h>

#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>

#include <odCore/Exception.h>
#include <odCore/physics/BulletAdapter.h>

namespace odPhysics
{

	ModelCollisionShape::ModelCollisionShape(size_t initialChildShapeCapacity)
	: btCompoundShape(true, initialChildShapeCapacity)
	{
	    mChildShapes.reserve(initialChildShapeCapacity);
	}

	void ModelCollisionShape::addManagedChildShape(btTransform xform, btCollisionShape *shape)
	{
	    mChildShapes.push_back(std::unique_ptr<btCollisionShape>(shape));

	    this->addChildShape(xform, shape);
	}


	ModelBounds::ModelBounds(ModelBounds::ShapeType type, size_t shapeCount)
	: mType(type)
	, mShapeCount(shapeCount)
	{
		mHierarchy.reserve(shapeCount);

		if(mType == BOXES)
		{
			mBoxes.reserve(shapeCount);

		}else
		{
			mSpheres.reserve(shapeCount);
		}
	}

	void ModelBounds::setMainBounds(const od::BoundingSphere &sphere, const od::OrientedBoundingBox &box)
	{
		mMainSphere = sphere;
		mMainBox = box;
	}

	void ModelBounds::addHierarchyEntry(uint16_t firstChildIndex, uint16_t nextSiblingIndex)
	{
		mHierarchy.push_back(std::make_pair(firstChildIndex, nextSiblingIndex));
	}

	void ModelBounds::addSphere(const od::BoundingSphere &sphere)
	{
		if(mType != SPHERES)
		{
			throw od::Exception("Can't add spheres to box-based model bounds");
		}

		mSpheres.push_back(sphere);
	}

	void ModelBounds::addBox(const od::OrientedBoundingBox &box)
	{
		if(mType != BOXES)
		{
			throw od::Exception("Can't add boxes to sphere-based model bounds");
		}

		mBoxes.push_back(box);
	}

	std::shared_ptr<ModelCollisionShape> ModelBounds::getSharedCollisionShape()
	{
	    if(mSharedShape == nullptr)
	    {
	        mSharedShape = buildNewShape();
	    }

	    return mSharedShape;
	}

	ModelCollisionShape *ModelBounds::buildNewShape()
	{
	    std::unique_ptr<ModelCollisionShape> shape(new ModelCollisionShape(mShapeCount));

        for(size_t index = 0; index < mShapeCount; ++index)
        {
            size_t firstChild = mHierarchy[index].first;
            size_t nextSibling = mHierarchy[index].second;
            glm::vec3 myTranslation;
            glm::quat myRotation;

            // Bullet does not seem to support a manual hierarchical bounds structure. Therefore, we only care about
            //  leafs in the bounding hierarchy here and ignore all shapes that have children
            if(firstChild == 0)
            {
                std::unique_ptr<btCollisionShape> newShape;

                if(mType == SPHERES)
                {
                    myTranslation = mSpheres[index].center();
                    myRotation = glm::quat(0,0,0,1);

                    newShape.reset(new btSphereShape(mSpheres[index].radius()));

                }else
                {
                    myTranslation = mBoxes[index].getBottomLeft() + mBoxes[index].getOrientation() * (mBoxes[index].getExtends() * 0.5);
                    myRotation = mBoxes[index].getOrientation();

                    btVector3 halfExtends = BulletAdapter::toBullet(mBoxes[index].getExtends() * 0.5); // btBoxShape wants half extends, so we multiply by 0.5
                    newShape.reset(new btBoxShape(halfExtends));
                }

                btTransform t = BulletAdapter::makeBulletTransform(myTranslation, myRotation);
                shape->addManagedChildShape(t, newShape.release());
            }
        }

        return shape.release();
	}

	void ModelBounds::printInfo()
	{
		_recursivePrint(0,0);
	}

	void ModelBounds::_recursivePrint(size_t index, size_t depth)
	{
		if(index >= mHierarchy.size())
		{
			return;
		}

		for(size_t i = 0; i < depth; ++i)
		{
			std::cout << "   |";
		}

		if(depth > 0)
		{
			std::cout << "-";
		}

		if(mType == BOXES)
		{
			od::OrientedBoundingBox &obb = mBoxes[index];

			std::cout << "[] x=" << obb.getBottomLeft().x << " y=" << obb.getBottomLeft().y << " z=" << obb.getBottomLeft().z
                      << " ex="<< obb.getExtends().x << " ey=" << obb.getExtends().y << " ez=" << obb.getExtends().z;

		}else
		{
			od::BoundingSphere &bs = mSpheres[index];

			std::cout << "()" << " r=" << bs.radius() << " x=" << bs.center().x << " y=" << bs.center().y << " z=" << bs.center().z;
		}

		std::cout << std::endl;

		size_t firstChild = mHierarchy[index].first;
		if(firstChild != 0)
			_recursivePrint(firstChild, depth+1);

		size_t nextSibling = mHierarchy[index].second;
		if(nextSibling != 0)
			_recursivePrint(nextSibling, depth);
	}

}
