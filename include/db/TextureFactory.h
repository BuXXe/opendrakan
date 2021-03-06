/*
 * TextureFactory.h
 *
 *  Created on: 2 Feb 2018
 *      Author: zal
 */

#ifndef INCLUDE_TEXTUREFACTORY_H_
#define INCLUDE_TEXTUREFACTORY_H_

#include "Asset.h"
#include "AssetFactory.h"
#include "FilePath.h"
#include "SrscFile.h"
#include "Texture.h"

namespace od
{

    class Engine;

    class TextureFactory : public AssetFactory<Texture>
	{
	public:

		struct PaletteColor
		{
			uint8_t red;
			uint8_t green;
			uint8_t blue;
			uint8_t dummy;
		};

		/**
         * This needs an engine instance because classes pass it to the RFL loaded hook.
         */
		TextureFactory(AssetProvider &ap, SrscFile &textureContainer, Engine &engine);

		inline Engine &getEngine() { return mEngine; }

		PaletteColor getPaletteColor(size_t index);


	protected:

		// implement AssetFactory<Texture>
		virtual osg::ref_ptr<Texture> loadAsset(RecordId textureId) override;


	private:

		void _loadPalette();

		Engine &mEngine;
		std::vector<PaletteColor> mPalette;
	};

}

#endif /* INCLUDE_TEXTUREFACTORY_H_ */
