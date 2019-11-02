/*
 * Class.h
 *
 *  Created on: 11 Feb 2018
 *      Author: zal
 */

#ifndef INCLUDE_CLASS_H_
#define INCLUDE_CLASS_H_

#include <memory>

#include <odCore/db/Asset.h>
#include <odCore/db/Model.h>
#include <odCore/rfl/Rfl.h>
#include <odCore/rfl/ClassBuilderProbe.h>

namespace od
{
    class LevelObject;
}

namespace odRfl
{
	class ClassRegistrar;
	class ClassBase;
	class LevelObjectClassBase;
	class Rfl;
	class RflManager;
}

namespace odDb
{
    class ClassFactory;

	class Class : public Asset
	{
	public:

		Class(AssetProvider &ap, od::RecordId classId, ClassFactory &classFactory);

		inline bool hasModel() const { return mModel != nullptr; }
        inline od::RefPtr<Model> getModel() { return mModel; }
        inline std::string getName() const { return mClassName; }
        inline odRfl::ClassId getRflClassId() const { return mRflClassId; }

        virtual void load(od::SrscFile::RecordInputCursor cursor) override;

        std::unique_ptr<odRfl::ClassBase> makeInstance(odRfl::RflManager &rflManager);
        std::unique_ptr<odRfl::LevelObjectClassBase> makeInstanceForLevelObject(odRfl::RflManager &rflManager, od::LevelObject &obj);

        /**
         * @brief Retrieves and
         */
        odRfl::ClassFactory *getFactory(odRfl::RflManager &manager);


	private:

        /**
         * @brief Returns cached registrar or attempts once to retrieve it from the RFL.
         */
        odRfl::ClassRegistrar *_getRegistrar(odRfl::RflManager &rflManager);

        ClassFactory &mClassFactory;

        std::string mClassName;
        AssetRef mModelRef;
        od::RefPtr<Model> mModel;
        uint16_t mRflClassId;
        odRfl::ClassBuilderProbe mClassBuilder;
        uint16_t mIconNumber;

        bool mTriedToCacheRegistrar;
        odRfl::ClassRegistrar *mCachedRflClassRegistrar;
        odRfl::Rfl *mRfl;

	};

	template <>
    struct AssetTraits<Class>
    {
        static const char *name()
        {
            return "Class";
        }

        static constexpr od::RecordType baseType()
        {
            return static_cast<od::RecordType>(od::SrscRecordType::CLASS);
        }
    };

}

#endif /* INCLUDE_CLASS_H_ */
