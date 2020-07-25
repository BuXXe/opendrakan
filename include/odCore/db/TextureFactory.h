/*
 * TextureFactory.h
 *
 *  Created on: 2 Feb 2018
 *      Author: zal
 */

#ifndef INCLUDE_TEXTUREFACTORY_H_
#define INCLUDE_TEXTUREFACTORY_H_

#include <odCore/FilePath.h>
#include <odCore/SrscFile.h>

#include <odCore/db/Asset.h>
#include <odCore/db/AssetFactory.h>
#include <odCore/db/Texture.h>

namespace odDb
{

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
		TextureFactory(AssetProvider &ap, od::SrscFile &textureContainer);

		PaletteColor getPaletteColor(size_t index);


	protected:

		// implement AssetFactory<Texture>
		virtual std::shared_ptr<Texture> createNewAsset(od::RecordId id) override;


	private:

		void _loadPalette();

		std::vector<PaletteColor> mPalette;
	};

}

#endif /* INCLUDE_TEXTUREFACTORY_H_ */
