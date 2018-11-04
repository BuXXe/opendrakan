/*
 * Model.h
 *
 *  Created on: 2 Feb 2018
 *      Author: zal
 */

#ifndef INCLUDE_MODEL_H_
#define INCLUDE_MODEL_H_

#include <memory>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <odCore/BoundingBox.h>
#include <odCore/physics/ModelBounds.h>
#include <odCore/db/Asset.h>
#include <odCore/db/Skeleton.h>

namespace odRender
{
    class RenderManager;
}

namespace odDb
{
    class ModelFactory;

	class Model : public Asset
	{
	public:

	    struct BoneAffection
        {
            size_t jointIndex;
            size_t vertexIndex;
            float vertexWeight;
        };

        struct Polygon
        {
            size_t vertexCount;
            size_t vertexIndices[4];
            glm::vec2 uvCoords[4];
            bool doubleSided;
            odDb::AssetRef texture;
        };

        struct LodMeshInfo
        {
            std::string lodName;
            float distanceThreshold;
            uint32_t usage;
            uint32_t nodeIndex;
            uint32_t firstVertexIndex;
            uint32_t vertexCount;
            uint32_t firstPolygonIndex;
            uint32_t polygonCount;

            std::vector<BoneAffection> boneAffections;
        };

        enum class ShadingType
        {
            None,
            Flat,
            Smooth
        };

		Model(AssetProvider &ap, od::RecordId modelId);
        Model(const Model &model) = delete; // models should never be copied as they can be reused throughout the scenegraph

		/// Returns SkeletonBuilder that can be used to construct a skeleton for an Object. May return nullptr if no skeleton present.
		inline SkeletonBuilder *getSkeletonBuilder() { return mSkeletonBuilder.get(); }
		inline odPhysics::ModelBounds *getModelBounds() { return mModelBounds.get(); }
		inline const std::vector<AssetRef> &getAnimationRefs() { return mAnimationRefs; }
		inline bool isCharacter() const { return mSkeletonBuilder != nullptr; }
		inline bool hasBounds() const { return mModelBounds != nullptr; }
		inline ShadingType getShadingType() const { return mShadingType; }

		void loadNameAndShading(ModelFactory &factory, od::DataReader &&dr);
		void loadVertices(ModelFactory &factory, od::DataReader &&dr);
		void loadTextures(ModelFactory &factory, od::DataReader &&dr);
		void loadPolygons(ModelFactory &factory, od::DataReader &&dr);
		void loadBoundingData(ModelFactory &factory, od::DataReader &&dr);
		void loadLodsAndBones(ModelFactory &factory, od::DataReader &&dr);

		/**
		 * @brief Returns an axis-aligned bounding box that encapsulates all of this model's meshes and LODs.
		 *
		 * This ignores any bounding info stored in the model. The box's expands are directly calculated from the vertex data.
		 */
		inline const od::AxisAlignedBoundingBox &getCalculatedBoundingBox() { return mCalculatedBoundingBox; }


	private:

		std::string mModelName;
		ShadingType mShadingType;
		bool mBlendWithLandscape;
		bool mShiny;
		bool mUseAdditiveBlending;
		bool mEnvironmentMapped;
		std::vector<glm::vec3> mVertices;
		std::vector<AssetRef> mTextureRefs;
		std::vector<Polygon> mPolygons;
		std::vector<LodMeshInfo> mLodMeshInfos;
		std::vector<AssetRef> mAnimationRefs;
		std::unique_ptr<odPhysics::ModelBounds> mModelBounds;
		std::unique_ptr<SkeletonBuilder> mSkeletonBuilder;
		bool mVerticesLoaded;
		bool mTexturesLoaded;
		bool mPolygonsLoaded;
		od::AxisAlignedBoundingBox mCalculatedBoundingBox;
	};

	template <>
    struct AssetTraits<Model>
    {
        static const char *name()
        {
            return "Model";
        }
    };

}

#endif /* INCLUDE_MODEL_H_ */
