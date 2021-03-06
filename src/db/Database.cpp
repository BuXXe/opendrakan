/*
 * RiotDb.cpp
 *
 *  Created on: 9 Jan 2018
 *      Author: zal
 */

#include "db/Database.h"

#include <fstream>
#include <sstream>
#include <regex>

#include "Logger.h"
#include "DbManager.h"
#include "StringUtils.h"
#include "Exception.h"

#define OD_RIOTDB_MAXVERSION 1

namespace od
{

	Database::Database(FilePath dbFilePath, DbManager &dbManager)
	: mDbFilePath(dbFilePath)
	, mDbManager(dbManager)
	, mVersion(0)
	{
	}

	Database::~Database()
	{
	}

	void Database::loadDbFileAndDependencies(size_t dependencyDepth)
	{
		std::regex versionRegex("\\s*version\\s+(\\d+).*");
		std::regex dependenciesRegex("\\s*dependencies\\s+(\\d+).*");
		std::regex dependencyDefRegex("\\s*(\\d+)\\s+(.*)");
		std::regex commentRegex("\\s*"); // allow empty lines. if we find something like a comment, add it here

		std::ifstream in(mDbFilePath.str(), std::ios::in | std::ios::binary);
		if(in.fail())
		{
		    throw IoException("Could not open db definition file " + mDbFilePath.str());
		}

		std::string line;
		bool readingDependencies = false;
		size_t totalDependencyCount = 0;
		size_t dependenciesRead = 0;
		while(std::getline(in, line))
		{
			// getline leaves the CR byte (0x0D) in the string if given windows line endings. remove if it is there
			if(line.size() != 0 && line[line.size() - 1] == 0x0D)
			{
				line.erase(line.size() - 1);
			}

			std::smatch results;

			if(std::regex_match(line, results, commentRegex))
			{
				continue;

			}else if(std::regex_match(line, results, versionRegex))
			{
				std::istringstream is(results[1]);
				is >> mVersion;

				if(mVersion > OD_RIOTDB_MAXVERSION)
				{
					throw UnsupportedException("Unsupported database version");
				}

			}else if(std::regex_match(line, results, dependenciesRegex))
			{
				std::istringstream is(results[1]);
				is >> totalDependencyCount;

				readingDependencies = true;

			}else if(std::regex_match(line, results, dependencyDefRegex))
			{
				if(!readingDependencies)
				{
					throw Exception("Found dependency definition before dependencies statement");
				}

				if(dependenciesRead >= totalDependencyCount)
                {
                    throw Exception("More dependency lines found in db file than stated in 'dependencies' statement");
                }

				uint32_t depIndex;
				std::istringstream is(results[1]);
				is >> depIndex;

				if(depIndex == 0)
				{
					throw Exception("Invalid dependency index");
				}

				// note: dependency paths are always stored relative to the path of the db file defining it
				FilePath depPath(results[2], mDbFilePath.dir());
				depPath = depPath.adjustCase();

				if(depPath == mDbFilePath)
				{
				    Logger::warn() << "Self dependent database file: " << mDbFilePath;
				    ++dependenciesRead;
				    continue;
				}

				Database &db = mDbManager.loadDb(depPath, dependencyDepth + 1);

				mDependencyMap.insert(std::pair<uint16_t, DbRefWrapper>(depIndex, db));

				++dependenciesRead;

			}else
			{
				throw Exception("Malformed line in database file: " + line);
			}
		}

        if(dependenciesRead < totalDependencyCount)
        {
            throw Exception("Found less dependency definitions than stated in dependencies statement");
        }


        // now that the database is loaded, create the various asset factories

        _tryOpeningAssetContainer(mClassFactory,    mClassContainer,    ".odb");
        _tryOpeningAssetContainer(mModelFactory,    mModelContainer,    ".mod");
        _tryOpeningAssetContainer(mAnimFactory,     mAnimContainer,     ".adb");
        _tryOpeningAssetContainer(mSoundFactory,    mSoundContainer,    ".sdb");
        _tryOpeningAssetContainer(mSequenceFactory, mSequenceContainer, ".ssd");

        // texture container is different. it needs an engine reference
        FilePath txdPath = mDbFilePath.ext(".txd");
        if(txdPath.exists())
        {
            mTextureContainer.reset(new SrscFile(txdPath));
            mTextureFactory.reset(new TextureFactory(*this, *mTextureContainer, mDbManager.getEngine()));

            Logger::verbose() << "Opened database texture container " << txdPath.str();

        }else
        {
            Logger::verbose() << "Database has no texture container";
        }
	}

	// TODO: the following methods look pretty redundant. find clever template interface for them
	Texture *Database::getTextureByRef(const AssetRef &ref)
	{
		if(ref.dbIndex == 0)
		{
			return this->getTexture(ref.assetId);
		}

		auto it = mDependencyMap.find(ref.dbIndex);
		if(it == mDependencyMap.end())
		{
			throw Exception("Database has no dependency with given index");
		}

		return it->second.get().getTexture(ref.assetId);
	}

	Class *Database::getClassByRef(const AssetRef &ref)
	{
		if(ref.dbIndex == 0)
		{
			return this->getClass(ref.assetId);
		}

		auto it = mDependencyMap.find(ref.dbIndex);
		if(it == mDependencyMap.end())
		{
			throw Exception("Database has no dependency with given index");
		}

		return it->second.get().getClass(ref.assetId);
	}

	Model *Database::getModelByRef(const AssetRef &ref)
	{
		if(ref.dbIndex == 0)
		{
			return this->getModel(ref.assetId);
		}

		auto it = mDependencyMap.find(ref.dbIndex);
		if(it == mDependencyMap.end())
		{
			throw Exception("Database has no dependency with given index");
		}

		return it->second.get().getModel(ref.assetId);
	}

    Sequence *Database::getSequenceByRef(const AssetRef &ref)
    {
        if(ref.dbIndex == 0)
        {
            return this->getSequence(ref.assetId);
        }

        auto it = mDependencyMap.find(ref.dbIndex);
        if(it == mDependencyMap.end())
        {
            throw Exception("Database has no dependency with given index");
        }

        return it->second.get().getSequence(ref.assetId);
    }

    Animation *Database::getAnimationByRef(const AssetRef &ref)
    {
    	if(ref.dbIndex == 0)
        {
            return this->getAnimation(ref.assetId);
        }

        auto it = mDependencyMap.find(ref.dbIndex);
        if(it == mDependencyMap.end())
        {
            throw Exception("Database has no dependency with given index");
        }

        return it->second.get().getAnimation(ref.assetId);
    }

    Sound *Database::getSoundByRef(const AssetRef &ref)
    {
        if(ref.dbIndex == 0)
        {
            return this->getSound(ref.assetId);
        }

        auto it = mDependencyMap.find(ref.dbIndex);
        if(it == mDependencyMap.end())
        {
            throw Exception("Database has no dependency with given index");
        }

        return it->second.get().getSound(ref.assetId);
    }


	Texture *Database::getTexture(RecordId recordId)
	{
		if(mTextureFactory == nullptr)
		{
			throw NotFoundException("Can't get texture. Database has no texture container");
		}

		osg::ref_ptr<Texture> asset = mTextureFactory->getAsset(recordId);

		return asset.release();
	}

	Class *Database::getClass(RecordId recordId)
	{
		if(mClassFactory == nullptr)
		{
			throw NotFoundException("Can't get class. Database has no class container");
		}

		osg::ref_ptr<Class> asset = mClassFactory->getAsset(recordId);

		return asset.release();
	}

	Model *Database::getModel(RecordId recordId)
	{
		if(mModelFactory == nullptr)
		{
			throw NotFoundException("Can't get model. Database has no model container");
		}

		osg::ref_ptr<Model> asset = mModelFactory->getAsset(recordId);

        return asset.release();
	}

	Sequence *Database::getSequence(RecordId recordId)
	{
        if(mSequenceFactory == nullptr)
        {
            throw NotFoundException("Can't get sequence. Database has no sequence container");
        }

        osg::ref_ptr<Sequence> asset = mSequenceFactory->getAsset(recordId);

        return asset.release();
	}

	Animation *Database::getAnimation(RecordId recordId)
	{
		if(mAnimFactory == nullptr)
		{
			throw NotFoundException("Can't get animation. Database has no animation container");
		}

		osg::ref_ptr<Animation> asset = mAnimFactory->getAsset(recordId);

		return asset.release();
	}

	Sound *Database::getSound(RecordId recordId)
    {
        if(mAnimFactory == nullptr)
        {
            throw NotFoundException("Can't get animation. Database has no animation container");
        }

        osg::ref_ptr<Sound> asset = mSoundFactory->getAsset(recordId);

        return asset.release();
    }

} 




