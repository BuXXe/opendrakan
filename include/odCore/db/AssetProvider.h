/*
 * AssetProvider.h
 *
 *  Created on: 11 Feb 2018
 *      Author: zal
 */

#ifndef INCLUDE_ASSETPROVIDER_H_
#define INCLUDE_ASSETPROVIDER_H_

#include <odCore/db/Asset.h>
#include <odCore/Exception.h>

namespace od
{

    class Texture;
    class Class;
    class Model;
    class Sequence;
    class Animation;
    class Sound;

	/**
	 * Common interface for classes that are able to provide assets, either directly or via indirection to
	 * dependency AssetProviders.
	 */
	class AssetProvider
	{
	public:

		virtual ~AssetProvider() = default;

		/**
		 * Gets a dependency of this provider with the given index. Implementers should override this if they
		 * can have dependencies. Default behaviour is to throw.
		 */
	    virtual AssetProvider &getDependency(uint16_t index);

	    /**
	     * Convenience method. This will redirect the call to getAsset() if the dbIndex in the passed reference is 0,
	     * otherwise it will try and resolve the dependency with using getDependency(), then call getAsset() on the
	     * returned dependency.
	     *
	     * @note This may throw an UnsupportedException depending on whether the implementation supports either having
	     * dependencies or directly providing assets.
	     */
	    template <typename _AssetType>
	    _AssetType *getAssetByRef(const AssetRef &ref)
	    {
	        if(ref.dbIndex == 0)
	        {
	            return getAsset<_AssetType>(ref.assetId);
	        }

	        AssetProvider &provider = getDependency(ref.dbIndex);

	        return provider.getAsset<_AssetType>(ref.assetId);
	    }

	    /**
	     * Synchronously loads an asset. This convenience template redirects the call to one of the
	     * virtual loading functions for supported assets.
	     *
	     * @note This might throw UnsupportedException if the \c _AssetType can't be loaded by this provider.
	     */
	    template <typename _AssetType>
	    _AssetType *getAsset(RecordId id);


	protected:

	    /**
         * Loads a texture synchronously without indirection. Implementing classes should override
         * this if they can provide textures. Default behaviour is to throw UnsupportedException.
         *
         * @note This transfers ownership. It is vital that the return value is stored in a ref_ptr.
         */
        virtual Texture   *getTexture(RecordId recordId);

        /**
         * Loads a class synchronously without indirection. Implementing classes should override
         * this if they can provide classes. Default behaviour is to throw UnsupportedException.
         *
         * @note This transfers ownership. It is vital that the return value is stored in a ref_ptr.
         */
        virtual Class     *getClass(RecordId recordId);

        /**
         * Loads a model synchronously without indirection. Implementing classes should override
         * this if they can provide models. Default behaviour is to throw UnsupportedException.
         *
         * @note This transfers ownership. It is vital that the return value is stored in a ref_ptr.
         */
        virtual Model     *getModel(RecordId recordId);

        /**
         * Loads a sequence synchronously without indirection. Implementing classes should override
         * this if they can provide sequences. Default behaviour is to throw UnsupportedException.
         *
         * @note This transfers ownership. It is vital that the return value is stored in a ref_ptr.
         */
        virtual Sequence  *getSequence(RecordId recordId);

        /**
         * Loads a animation synchronously without indirection. Implementing classes should override
         * this if they can provide animations. Default behaviour is to throw UnsupportedException.
         *
         * @note This transfers ownership. It is vital that the return value is stored in a ref_ptr.
         */
        virtual Animation *getAnimation(RecordId recordId);

        /**
         * Loads a sound synchronously without indirection. Implementing classes should override
         * this if they can provide sounds. Default behaviour is to throw UnsupportedException.
         *
         * @note This transfers ownership. It is vital that the return value is stored in a ref_ptr.
         */
        virtual Sound     *getSound(RecordId recordId);

	};

	/*template<>
    Texture *AssetProvider::getAssetByRef<Texture>(const AssetRef &ref);

	template<>
    Class *AssetProvider::getAssetByRef<Class>(const AssetRef &ref);

	template<>
    Model *AssetProvider::getAssetByRef<Model>(const AssetRef &ref);

	template<>
    Sequence *AssetProvider::getAssetByRef<Sequence>(const AssetRef &ref);

	template<>
    Animation *AssetProvider::getAssetByRef<Animation>(const AssetRef &ref);

	template<>
    Sound *AssetProvider::getAssetByRef<Sound>(const AssetRef &ref);*/


}

#endif /* INCLUDE_ASSETPROVIDER_H_ */
