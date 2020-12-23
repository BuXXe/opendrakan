/*
 * Model.cpp
 *
 *  Created on: 2 Feb 2018
 *      Author: zal
 */

#include <odCore/db/Model.h>

#include <algorithm>
#include <limits>

#include <odCore/Exception.h>

#include <odCore/db/Asset.h>
#include <odCore/db/ModelFactory.h>
#include <odCore/db/SkeletonBuilder.h>
#include <odCore/db/Texture.h>
#include <odCore/db/ModelBounds.h>

#include <odCore/render/Renderer.h>
#include <odCore/render/Model.h>

#include <odCore/physics/PhysicsSystem.h>
#include <odCore/physics/ModelShape.h>

#define OD_POLYGON_FLAG_DOUBLESIDED 0x02

namespace odDb
{

	Model::Model()
	: mModelName("")
	, mShadingType(ShadingType::None)
	, mBlendWithLandscape(false)
	, mShiny(false)
	, mUseAdditiveBlending(false)
	, mEnvironmentMapped(false)
	, mVerticesLoaded(false)
	, mTexturesLoaded(false)
	, mPolygonsLoaded(false)
	{
	}

	Model::~Model()
	{
	}

	const ModelBounds &Model::getModelBounds(size_t lodIndex)
	{
	    if(lodIndex >= mModelBounds.size())
	    {
	        throw od::Exception("LOD index for getting model bounds out of bounds. Lol");
	    }

	    return mModelBounds[lodIndex];
	}

	void Model::load(od::SrscFile::RecordInputCursor cursor)
	{
	    auto nameRecordIt = cursor.getDirIterator();

        // required records
        _loadNameAndShading(cursor.getReader());

        if(!cursor.nextOfTypeId(od::SrscRecordType::MODEL_VERTICES, getAssetId(), 8))
        {
            throw od::Exception("Found no vertex record after model name record");
        }
        _loadVertices(cursor.getReader());

        cursor.moveTo(nameRecordIt);
        if(!cursor.nextOfTypeId(od::SrscRecordType::MODEL_TEXTURES, getAssetId(), 8))
        {
            throw od::Exception("Found no texture record after model name record");
        }
        _loadTextures(cursor.getReader());

        cursor.moveTo(nameRecordIt);
        if(!cursor.nextOfTypeId(od::SrscRecordType::MODEL_POLYGONS, getAssetId(), 8))
        {
            throw od::Exception("Found no polyon record after model name record");
        }
        _loadPolygons(cursor.getReader());

        // optional records
        cursor.moveTo(nameRecordIt);
        if(cursor.nextOfTypeId(od::SrscRecordType::MODEL_LOD_BONES, getAssetId(), 8))
        {
            _loadLodsAndBones(cursor.getReader());
        }

        cursor.moveTo(nameRecordIt);
        if(cursor.nextOfTypeId(od::SrscRecordType::MODEL_BOUNDING, getAssetId(), 8))
        {
            _loadBoundingData(cursor.getReader());
        }
	}

	void Model::_loadNameAndShading(od::DataReader dr)
    {
        dr >> mModelName;

        uint32_t shadingType;
        dr >> shadingType;
        if((shadingType & 0x02) && !(shadingType & 0x01))
        {
            mShadingType = ShadingType::Smooth;

        }else if((shadingType & 0x01) && !(shadingType & 0x02))
        {
            mShadingType = ShadingType::Flat;

        }else
        {
            // if none or both flags are set, disable shading (the latter case as a failsafe)
            mShadingType = ShadingType::None;
        }

        mEnvironmentMapped = shadingType & 0x20;
        mUseAdditiveBlending = shadingType & 0x10;
        mShiny = shadingType & 0x08;
        mBlendWithLandscape = shadingType & 0x04;
    }

    void Model::_loadVertices(od::DataReader dr)
    {
        uint16_t vertexCount;
        dr >> vertexCount;

        mVertices.resize(vertexCount);
        for(size_t i = 0; i < vertexCount; ++i)
        {
            dr >> mVertices[i];

            mCalculatedBoundingBox.expandBy(mVertices[i]);
            mCalculatedBoundingSphere.expandBy(mVertices[i]);
        }

        mVerticesLoaded = true;
    }

    void Model::_loadTextures(od::DataReader dr)
    {
        uint32_t textureCount;
        dr >> textureCount;

        mTextureRefs.reserve(textureCount);
        for(size_t i = 0; i < textureCount; ++i)
        {
            AssetRef textureRef;
            dr >> textureRef;

            if(!textureRef.isNull())
            {
                mTextureRefs.push_back(textureRef);
            }
        }

        mTexturesLoaded = true;
    }

    void Model::_loadPolygons(od::DataReader dr)
    {
        if(!mTexturesLoaded || !mVerticesLoaded)
        {
            throw od::Exception("Must load vertices and textures before loading polygons!");
        }

        uint16_t polygonCount;
        dr >> polygonCount;

        mPolygons.reserve(polygonCount);
        for(size_t i = 0; i < polygonCount; ++i)
        {
            uint16_t flags;
            uint16_t textureIndex;
            uint16_t vertexCount;

            dr >> flags
               >> vertexCount
               >> textureIndex;

            Polygon poly;
            poly.doubleSided = flags & OD_POLYGON_FLAG_DOUBLESIDED;
            poly.texture = mTextureRefs[textureIndex];
            poly.vertexCount = vertexCount;

            if(poly.vertexCount != 3 && poly.vertexCount != 4)
            {
                throw od::UnsupportedException("Can't load model with non-triangle/non-quad primitives");
            }

            for(size_t i = 0; i < poly.vertexCount; ++i)
            {
                uint16_t vertexIndex;

                dr >> vertexIndex
                   >> poly.uvCoords[i];

                poly.vertexIndices[i] = vertexIndex; // TODO: instead of doing it this way, maybe add a "readAs<type>" modifier?
            }

            mPolygons.push_back(poly);
        }

        mPolygonsLoaded = true;
    }

    void Model::_loadBoundingData(od::DataReader dr)
    {
        od::BoundingSphere mainBs;
        od::OrientedBoundingBox mainObb;
        uint16_t shapeCount;
        uint16_t shapeType; // 0 = spheres, 1 = boxes

        dr >> mainBs
           >> mainObb
           >> shapeCount
           >> shapeType;

        ModelBounds::ShapeType type = (shapeType == 0) ? ModelBounds::SPHERES : ModelBounds::BOXES;
        mModelBounds.emplace_back(type, shapeCount);

        ModelBounds &bounds = mModelBounds.back();
        bounds.setMainBounds(mainBs, mainObb);

        for(size_t i = 0; i < shapeCount; ++i)
        {
            uint16_t firstChildIndex;
            uint16_t nextSiblingIndex;
            dr >> firstChildIndex
               >> nextSiblingIndex;

            bounds.addHierarchyEntry(firstChildIndex, nextSiblingIndex);
        }

        for(size_t i = 0; i < shapeCount; ++i)
        {
            if(shapeType == 0)
            {
                od::BoundingSphere bs;
                dr >> bs;
                bounds.addSphere(bs);

            }else
            {
                od::OrientedBoundingBox obb;
                dr >> obb;
                bounds.addBox(obb);

                // ignore the field of words after each box
                uint16_t unkPolyCount;
                dr >> unkPolyCount;
                dr.ignore(unkPolyCount*2);
            }
        }

        //Logger::info() << "Bounding data for model " << mModelName;
        //mModelBounds->printInfo();
    }

    void Model::_loadLodsAndBones(od::DataReader dr)
    {
        // main bounds/culling bounds. actual collision bounds follow later
        od::BoundingSphere mainSphere;
        od::OrientedBoundingBox mainObb;
        dr >> mainSphere >> mainObb;

        uint16_t lodCount;
        std::vector<std::string> lodNames;

        dr >> lodCount;

        if(lodCount == 0)
        {
            throw od::Exception("Expected at least one LOD in model");
        }
        mLodMeshInfos.resize(lodCount);

        lodNames.resize(lodCount - 1);
        for(int32_t i = 0; i < lodCount - 1; ++i)
        {
            dr >> lodNames[i];
        }


        mSkeletonBuilder = std::make_unique<SkeletonBuilder>();

        // node info
        uint16_t nodeInfoCount;
        dr >> nodeInfoCount;
        for(size_t i = 0; i < nodeInfoCount; ++i)
        {
            char nodeName[33] = { 0 };
            int32_t jointInfoIndex;

            dr.read(nodeName, 32);
            dr >> jointInfoIndex;

            mSkeletonBuilder->addJointNameInfo(std::string(nodeName), jointInfoIndex);
        }

        // joint info
        uint16_t jointInfoCount;
        dr >> jointInfoCount;
        for(size_t jointIndex = 0; jointIndex < jointInfoCount; ++jointIndex)
        {
            glm::mat3x4 inverseBoneTransform;
            int32_t meshIndex;
            int32_t firstChildIndex;
            int32_t nextSiblingIndex;

            dr >> inverseBoneTransform
               >> meshIndex
               >> firstChildIndex
               >> nextSiblingIndex;

            mSkeletonBuilder->addJointInfo(glm::mat4(inverseBoneTransform), meshIndex, firstChildIndex, nextSiblingIndex);

            // affected vertex lists, one for each LOD
            for(size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
            {
                std::vector<BoneAffection> &boneAffections = mLodMeshInfos[lodIndex].boneAffections;

                uint16_t affectedVertexCount;
                dr >> affectedVertexCount;
                boneAffections.reserve(boneAffections.size() + affectedVertexCount);
                for(size_t vertexIndex = 0; vertexIndex < affectedVertexCount; ++vertexIndex)
                {
                    uint32_t affectedVertexIndex;
                    float weight;
                    dr >> affectedVertexIndex
                       >> weight;

                    BoneAffection bAff;
                    bAff.jointIndex = jointIndex;
                    bAff.vertexIndex = affectedVertexIndex;
                    bAff.vertexWeight = weight;
                    boneAffections.push_back(bAff);
                }
            }
        }

        // lod info
        for(size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
        {
            uint16_t meshCount;
            dr >> meshCount;

            if(meshCount != 1)
            {
                throw od::UnsupportedException("Multi-mesh-models currently unsupported");
            }

            for(size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
            {
                LodMeshInfo &mesh = mLodMeshInfos[lodIndex];

                dr >> mesh.distanceThreshold
                   >> mesh.usage
                   >> mesh.nodeIndex
                   >> mesh.firstVertexIndex
                   >> mesh.vertexCount
                   >> mesh.firstPolygonIndex
                   >> mesh.polygonCount;

                mesh.lodName = (lodIndex == 0) ? mModelName : lodNames[lodIndex - 1];
            }
        }

        // animations refs
        uint16_t animCount;
        dr >> animCount;
        mAnimationRefs.resize(animCount);
        for(size_t animIndex = 0; animIndex < animCount; ++animIndex)
        {
            // the database index of this is always zero. however, i see no reason not to support loading
            //  animations cross-database, so we don't enforce this.
            dr >> mAnimationRefs[animIndex];
        }


        // channels
        uint16_t channelCount;
        dr >> channelCount;
        for(size_t channelIndex = 0; channelIndex < channelCount; ++channelIndex)
        {
            uint32_t jointIndex;
            glm::mat3x4 xformA;
            glm::mat3x4 xformB;
            uint16_t capCount;

            dr >> jointIndex
               >> xformA
               >> xformB
               >> capCount;

            mSkeletonBuilder->markJointAsChannel(jointIndex);

            for(size_t capIndex = 0; capIndex < capCount; ++capIndex)
            {
                uint32_t firstCapPolygonIndex;
                uint32_t capPolygonCount;
                uint32_t firstPartPolygonIndex;
                uint32_t partPolygonCount;
                uint32_t unk;
                uint16_t affectedVertexCount;

                dr >> firstCapPolygonIndex
                   >> capPolygonCount
                   >> firstPartPolygonIndex
                   >> partPolygonCount
                   >> unk
                   >> affectedVertexCount;

                for(size_t vertexIndex = 0; vertexIndex < affectedVertexCount; ++vertexIndex)
                {
                    uint32_t affectedVertexIndex;
                    float weight;
                    dr >> affectedVertexIndex
                       >> weight;
                }
            }
        }

        //Logger::info() << "Skel info for model " << mModelName;
        //mSkeletonBuilder->printInfo(std::cout);

        dr >> od::DataReader::Expect<uint16_t>(lodCount);

        mModelBounds.reserve(lodCount);

        for(size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
        {
            uint16_t sphereCount;
            dr >> sphereCount;

            mModelBounds.emplace_back(ModelBounds::SPHERES, sphereCount);
            ModelBounds &bounds = mModelBounds.back();
            bounds.setMainBounds(mainSphere, mainObb);

            for(size_t sphereIndex = 0; sphereIndex < sphereCount; ++sphereIndex)
            {
                uint16_t firstChild;
                uint16_t nextSibling;
                uint32_t positionVertexIndex;
                float radius;
                uint16_t channelIndex;

                dr >> firstChild
                   >> nextSibling
                   >> positionVertexIndex
                   >> radius
                   >> channelIndex
                   >> od::DataReader::Ignore(2);

                if(positionVertexIndex >= mVertices.size())
                {
                    Logger::warn() << "Character bounding sphere center vertex index out of bounds. PC LOAD LETTER";

                }else
                {
                    bounds.addHierarchyEntry(firstChild, nextSibling);

                    glm::vec3 sphereCenter = mVertices[positionVertexIndex];
                    od::BoundingSphere sphere(sphereCenter, radius);
                    bounds.addSphere(sphere);
                }
            }
        }
    }
}
