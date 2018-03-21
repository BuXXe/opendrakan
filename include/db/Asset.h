/*
 * Asset.h
 *
 *  Created on: 2 Feb 2018
 *      Author: zal
 */

#ifndef INCLUDE_ASSET_H_
#define INCLUDE_ASSET_H_

#include "SrscFile.h"

namespace od
{

	class Database;

	class Asset
	{
	public:

		Asset(Database &db, RecordId assetId);
		Asset(Asset &a) = delete;
		Asset(const Asset &a) = delete;
		virtual ~Asset();

		inline RecordId getAssetId() const { return mId; }
		inline Database &getDatabase() { return mDb; }

		virtual const char *getAssetTypeName() const = 0;

		/// loader function for assets that are loadable from a single record. used by the generic asset factory. default implementation throws
		virtual void loadFromRecord(DataReader &dr);


	private:

		Database &mDb;
		RecordId mId;

	};

	/**
	 * Structure for representing a Record-ID & Database-Index pair as they
	 * appear all over the engines files when referencing another asset.
	 */
	struct AssetRef
	{
		AssetRef() : assetId(0), dbIndex(0) {}
		AssetRef(RecordId id, uint16_t index) : assetId(id), dbIndex(index) {}

		RecordId assetId;
		uint16_t dbIndex;

		// so we can use this in a map
		bool operator<(const AssetRef &right) const;

		bool operator==(const AssetRef &right) const;
		inline bool operator!=(const AssetRef &right) const { return !(this->operator==(right)); }

		/**
		 * @brief Checks if this ref is a null texture (layer textures use 0xffff:0xffff for none instead of 0:0)
		 */
		inline bool isNullTexture() const { return dbIndex == 0xffff || assetId == 0xffff; }

		/**
		 * @brief Checks if this is a null reference i.e. not referencing anything. Not applicable to layer textures.
		 */
		inline bool isNull() const { return dbIndex == 0 && assetId == 0; }

		static const AssetRef NULL_REF;
	};

	DataReader &operator>>(DataReader &left, AssetRef &right);

	std::ostream &operator<<(std::ostream &left, const AssetRef &right);

}

#endif /* INCLUDE_ASSET_H_ */