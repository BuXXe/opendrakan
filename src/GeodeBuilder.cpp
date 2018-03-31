/*
 * GeodeBuilder.cpp
 *
 *  Created on: 26 Mar 2018
 *      Author: zal
 */

#include <GeodeBuilder.h>
#include <algorithm>
#include <osg/Geometry>

#include "Exception.h"
#include "db/Database.h"

namespace od
{

	GeodeBuilder::GeodeBuilder(const std::string &modelName, AssetProvider &assetProvider)
	: mModelName(modelName)
	, mAssetProvider(assetProvider)
	, mClampTextures(false)
	{
	}

	void GeodeBuilder::setVertexVector(std::vector<osg::Vec3f>::iterator begin, std::vector<osg::Vec3f>::iterator end)
	{
		if(mVertices != nullptr)
		{
			Logger::warn() << "Double set vertex vector for geode building. Ignoring second call";
			return;
		}

		mVertices = new osg::Vec3Array;
		mVertices->assign(begin, end);
	}

	void GeodeBuilder::setPolygonVector(std::vector<Polygon>::iterator begin, std::vector<Polygon>::iterator end)
	{
		// need to iterate over polygons and convert quads to triangles and duplicate polygon if it is double-sided

		mTriangles.clear();
		mTriangles.reserve(end - begin); // assuming we only have single-sided triangles, this is the minimum we have to allocate

		for(auto it = begin; it != end; ++it)
		{
			if(it->vertexCount != 3 && it->vertexCount != 4)
			{
				throw UnsupportedException("Only triangle or quad polygons supported");
			}

			// the 0 1 2 triangle always appears
			Triangle tri;
			tri.texture = it->texture;
			tri.uvCoords[0] = it->uvCoords[0];
			tri.uvCoords[1] = it->uvCoords[1];
			tri.uvCoords[2] = it->uvCoords[2];
			tri.vertexIndices[0] = it->vertexIndices[0];
			tri.vertexIndices[1] = it->vertexIndices[1];
			tri.vertexIndices[2] = it->vertexIndices[2];
			mTriangles.push_back(tri);

			if(it->doubleSided)
			{
				// swapping verts 0 and 2 reverses winding order, thus flipping the triangle
				std::swap(tri.vertexIndices[0], tri.vertexIndices[2]);
				std::swap(tri.uvCoords[0], tri.uvCoords[2]);
				mTriangles.push_back(tri);
			}

			// if the poly is a quad, add another triangle
			if(it->vertexCount == 4)
			{
				tri.uvCoords[0] = it->uvCoords[0];
				tri.uvCoords[1] = it->uvCoords[2];
				tri.uvCoords[2] = it->uvCoords[3];
				tri.vertexIndices[0] = it->vertexIndices[0];
				tri.vertexIndices[1] = it->vertexIndices[2];
				tri.vertexIndices[2] = it->vertexIndices[3];
				mTriangles.push_back(tri);

				if(it->doubleSided)
				{
					std::swap(tri.vertexIndices[0], tri.vertexIndices[2]);
					std::swap(tri.uvCoords[0], tri.uvCoords[2]);
					mTriangles.push_back(tri);
				}
			}
		}
	}

	void GeodeBuilder::setBoneAffectionVector(std::vector<BoneAffection>::iterator begin, std::vector<BoneAffection>::iterator end)
	{
		// here we turn the BoneAffection objects into the index and weight vectors

		if(mBoneIndices != nullptr || mBoneWeights != nullptr)
		{
			Logger::warn() << "Double set bone affection vector for geode building. Ignoring second call";
			return;
		}

		if(mVertices == nullptr)
		{
			throw Exception("Need to add vertex vector to GeodeBuilder before adding bone affections");
		}

		mBoneIndices = new osg::Vec4Array;
		mBoneIndices->setName("influencingBones");
		mBoneIndices->setBinding(osg::Array::BIND_PER_VERTEX);
		mBoneIndices->resize(mVertices->size(), osg::Vec4(0, 0, 0, 0));

		mBoneWeights = new osg::Vec4Array;
		mBoneWeights->setName("vertexWeights");
		mBoneWeights->setBinding(osg::Array::BIND_PER_VERTEX);
		mBoneWeights->resize(mVertices->size(), osg::Vec4f(0, 0, 0, 0)); // weight of 0 will make unused bones uneffective, regardless
																         //  of index -> less logic in the vertex shader!
		std::vector<size_t> influencingBonesCount(mVertices->size(), 0);

		for(auto it = begin; it != end; ++it)
		{
			if(it->vertexIndex >= mVertices->size())
			{
				Logger::error() << "Affected vertex index of bone out of bounds. Was " << it->vertexIndex << " where size was " << mVertices->size();
				throw Exception("Affected vertex index of bone out of bounds");
			}

			size_t &currentBoneCount = influencingBonesCount[it->vertexIndex];
			if(currentBoneCount >= 4)
			{
				Logger::warn() << "More than 4 bones per vertex";
				// TODO: perhaps overwrite bone with lowest weight rather than ignoring all bones past the fourth?
				continue;
			}

			mBoneIndices->at(it->vertexIndex)[currentBoneCount] = it->jointIndex;
			mBoneWeights->at(it->vertexIndex)[currentBoneCount] = it->vertexWeight;
			++currentBoneCount;
		}
	}

	void GeodeBuilder::build(osg::Geode *geode)
	{
		if(mVertices == nullptr)
		{
			throw Exception("Need to add vertex vector to GeodeBuilder before building");
		}

		_buildNormals();
		_disambiguateAndGenerateUvs();

		// sort by texture. most models are already sorted, so this is O(n) most of the time
		auto pred = [](Triangle &left, Triangle &right){ return left.texture < right.texture; };
		std::sort(mTriangles.begin(), mTriangles.end(), pred);

		osg::ref_ptr<osg::Geometry> geom;
		osg::ref_ptr<osg::DrawElementsUInt> drawElements;
		AssetRef lastTexture = AssetRef::NULL_REF;
		for(auto it = mTriangles.begin(); it != mTriangles.end(); ++it)
		{
			if(lastTexture != it->texture || geom == nullptr)
			{
				geom = new osg::Geometry;
				geode->addDrawable(geom);

				geom->setUseVertexBufferObjects(true);
				geom->setUseDisplayList(false);

				// shared VBOs
				geom->setVertexArray(mVertices);
				geom->setNormalArray(mNormals);
				geom->setTexCoordArray(0, mUvCoords);
				if(mBoneIndices != nullptr && mBoneWeights != nullptr)
				{
					// FIXME: these locations may get inconsistent with what we use in the shader
					geom->setVertexAttribArray(14, mBoneIndices, osg::Array::BIND_PER_VERTEX);
					geom->setVertexAttribArray(15, mBoneWeights, osg::Array::BIND_PER_VERTEX);
				}

				// IBO unique per geometry/texture
				drawElements = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES); // TODO: reserve indices to reduce reallocation
				geom->addPrimitiveSet(drawElements);

				// texture also unique per geometry
				if(!it->texture.isNullTexture())
				{
					TexturePtr textureImage = mAssetProvider.getTextureByRef(it->texture);
					osg::StateSet *ss = geom->getOrCreateStateSet();
					if(textureImage->hasAlpha())
					{
						ss->setMode(GL_BLEND, osg::StateAttribute::ON);
						ss->setRenderBinDetails(1, "DepthSortedBin");
					}

					osg::ref_ptr<osg::Texture2D> texture(new osg::Texture2D(textureImage));
					if(!mClampTextures)
					{
						// this is the default for model textures
						texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
						texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);

					}else
					{
						// for layers we should use clamp to border instead
						texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
						texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
					}
					ss->setTextureAttributeAndModes(0, texture);
				}

				lastTexture = it->texture;
			}

			for(size_t vn = 0; vn < 3; ++vn)
			{
				drawElements->push_back(it->vertexIndices[vn]);
			}
		}
	}

	void GeodeBuilder::_buildNormals()
	{
		// calculate normals per triangle, sum them up for each vertex and normalize them in the end
		//  TODO: perhaps consider maximum crease here?

		mNormals = new osg::Vec3Array;
		mNormals->resize(mVertices->size(), osg::Vec3(0,0,0));

		for(auto it = mTriangles.begin(); it != mTriangles.end(); ++it)
		{
			// note: Drakan uses CW orientation!
			osg::Vec3 normal =   (mVertices->at(it->vertexIndices[2]) - mVertices->at(it->vertexIndices[0]))
							   ^ (mVertices->at(it->vertexIndices[1]) - mVertices->at(it->vertexIndices[0]));

			mNormals->at(it->vertexIndices[0]) += normal;
			mNormals->at(it->vertexIndices[1]) += normal;
			mNormals->at(it->vertexIndices[2]) += normal;
		}

		for(auto it = mNormals->begin(); it != mNormals->end(); ++it)
		{
			it->normalize();
		}
	}

	void GeodeBuilder::_disambiguateAndGenerateUvs()
	{
		// iterate over triangles and duplicate shared vertices when their uv coordinates are incompatible.
		//  NOTE: we share VBOs between all geometries. only IBOs are unique per texture. thus, we only need to
		//         make sure UVs are unique per vertex, not textures.

		mUvCoords = new osg::Vec2Array;
		mUvCoords->resize(mVertices->size(), osg::Vec2f(0, 0));

		std::vector<bool> visited(mVertices->size(), false);

		for(auto it = mTriangles.begin(); it != mTriangles.end(); ++it)
		{
			for(size_t vn = 0; vn < 3; ++vn)
			{
				size_t &vertIndex = it->vertexIndices[vn];

				// if we haven't touched the vertex yet, assign uv coordinates to it. if we have touched it,
				//  their uvs must match. otherwise, duplicate it and adjust the indices in the triangles
				if(!visited[vertIndex])
				{
					mUvCoords->at(vertIndex) = it->uvCoords[vn];
					visited[vertIndex] = true;

				}else if(mUvCoords->at(vertIndex) != it->uvCoords[vn])
				{
					// incompatible UVs. duplicate vertex and normal
					mVertices->push_back(mVertices->at(vertIndex));
					mNormals->push_back(mNormals->at(vertIndex));

					// if there is bone affection info, duplicate that too
					if(mBoneIndices != nullptr && mBoneIndices->size() > vertIndex)
					{
						mBoneIndices->push_back(mBoneIndices->at(vertIndex));
						mBoneWeights->push_back(mBoneWeights->at(vertIndex));
					}

					// the only thing that's different are uv coords:
					mUvCoords->push_back(it->uvCoords[vn]);

					// lastly, make sure the triangle uses that new vertex
					vertIndex = mVertices->size() - 1;
				}
			}
		}
	}
}
