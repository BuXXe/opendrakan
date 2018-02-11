/*
 * SegmentedGeode.h
 *
 *  Created on: 10 Feb 2018
 *      Author: zal
 */

#ifndef INCLUDE_SEGMENTEDGEODE_H_
#define INCLUDE_SEGMENTEDGEODE_H_

#include <vector>
#include <osg/Geode>

#include "Asset.h"

namespace od
{

	class AssetProvider;

	/**
	 * Geode for constructing geometries with mutiple textures that splits
	 * them into segments with the same texture and converts quads to triangles.
	 *
	 * Construction is O(n*log(n)).
	 */
	class SegmentedGeode : public osg::Geode
	{
	public:

		struct Face
		{
			size_t    vertexCount;
			AssetRef  texture;
			size_t    vertexIndices[4];
        	osg::Vec2 vertexUvCoords[4];
		};

	protected:

		void build(AssetProvider &db, std::vector<osg::Vec3> &vertexArray, std::vector<Face> &faceArray, size_t textureCount = 0);

	};

}

#endif /* INCLUDE_SEGMENTEDGEODE_H_ */