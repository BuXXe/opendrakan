/*
 * RiotLevel.cpp
 *
 *  Created on: 25 Jan 2018
 *      Author: zal
 */


#include <odCore/Level.h>

#include <algorithm>
#include <osg/Depth>

#include <odCore/OdDefines.h>
#include <odCore/SrscRecordTypes.h>
#include <odCore/SrscFile.h>
#include <odCore/Logger.h>
#include <odCore/ZStream.h>
#include <odCore/Exception.h>
#include <odCore/Engine.h>
#include <odCore/LevelObject.h>

namespace od
{

    Level::Level(const FilePath &levelPath, Engine &engine, osg::ref_ptr<osg::Group> levelRootNode)
    : mLevelPath(levelPath)
    , mEngine(engine)
    , mDbManager(engine.getDbManager())
    , mMaxWidth(0)
    , mMaxHeight(0)
    , mLevelRootNode(levelRootNode)
    , mLayerGroup(new osg::Group)
    , mObjectGroup(new osg::Group)
    , mPhysicsManager(*this, levelRootNode)
    {
    	mLevelRootNode->addChild(mLayerGroup);
    	mLevelRootNode->addChild(mObjectGroup);

		mLevelRootNode->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    }

    Level::~Level()
    {
    	// despawn all remaining objects
    	for(auto it = mLevelObjects.begin(); it != mLevelObjects.end(); ++it)
    	{
    		(*it)->despawned();
    	}

    	// TODO: this needs a proper mechanism and a map~ we will be doing this on the fly later
    	for(auto it = mLayers.begin(); it != mLayers.end(); ++it)
    	{
    		mPhysicsManager.removeLayer(*it->get());
    	}

    	mLevelRootNode->removeChild(mLayerGroup);
        mLevelRootNode->removeChild(mObjectGroup);
    }

    void Level::loadLevel()
    {
        Logger::info() << "Loading level " << mLevelPath.str();

        SrscFile file(mLevelPath);

        _loadNameAndDeps(file);
        _loadLayers(file);
        //_loadLayerGroups(file); unnecessary, as this is probably just an editor thing
        _loadObjects(file);

        Logger::info() << "Level loaded successfully";
    }

    void Level::spawnAllObjects()
    {
        Logger::info() << "Spawning all objects for debugging (conditional spawning not implemented yet)";
        for(auto it = mLevelObjects.begin(); it != mLevelObjects.end(); ++it)
        {
            mObjectGroup->addChild(*it);
            (*it)->spawned();
        }
    }

    void Level::requestLevelObjectDestruction(LevelObject *obj)
    {
        mDestructionQueue.push_back(osg::ref_ptr<LevelObject>(obj));
    }

    Layer *Level::getLayerById(uint32_t id)
    {
        auto pred = [id](osg::ref_ptr<Layer> &l){ return l->getId() == id; };

        auto it = std::find_if(mLayers.begin(), mLayers.end(), pred);
        if(it == mLayers.end())
        {
            return nullptr;
        }

        return it->get();
    }

    Layer *Level::getLayerByIndex(uint16_t index)
    {
        if(index >= mLayers.size())
        {
            return nullptr;
        }

        return mLayers[index].get();
    }

    Layer *Level::getFirstLayerBelowPoint(const osg::Vec3 &v)
    {
        // TODO: use an efficient spatial search here
        //  using brute force for now

        osg::Vec2 xz(v.x(), v.z());

        // first, find all candidate layers that overlap the given point in the xz plane
        mLayerLookupCache.clear();
        for(auto it = mLayers.begin(); it != mLayers.end(); ++it)
        {
            if((*it)->contains(xz))
            {
                mLayerLookupCache.push_back(*it);
            }
        }

        // then, find the layer with smallest, negative height difference among those candiates
        Layer *closestLayer = nullptr;
        float minDistance = std::numeric_limits<float>::max();
        for(auto it = mLayerLookupCache.begin(); it != mLayerLookupCache.end(); ++it)
        {
            if((*it)->hasHoleAt(xz))
            {
                continue;
            }

            float heightDifferenceDown = v.y() - (*it)->getAbsoluteHeightAt(xz);
            if(heightDifferenceDown < 0)
            {
                continue; // don't care about layers above us
            }

            if(heightDifferenceDown < minDistance)
            {
                minDistance = heightDifferenceDown;
                closestLayer = *it;
            }
        }

        return closestLayer;
    }

    void Level::update()
    {
        if(!mDestructionQueue.empty())
        {
            auto it = mDestructionQueue.begin();
            while(it != mDestructionQueue.end())
            {
                (*it)->despawned();
                (*it)->destroyed();
                mObjectGroup->removeChild(*it);

                it = mDestructionQueue.erase(it);
            }
        }
    }

    LevelObject &Level::getLevelObjectByIndex(uint16_t index)
    {
        if(index >= mLevelObjects.size())
        {
            throw NotFoundException("Level object with given index not found");
        }

        return *mLevelObjects[index];
    }

    // TODO: the following methods look pretty redundant. find clever template interface for them
    Texture *Level::getTextureByRef(const AssetRef &ref)
	{
    	Logger::debug() << "Requested texture " << std::hex << ref.assetId << std::dec << " from level dependency " << ref.dbIndex;

        auto it = mDependencyMap.find(ref.dbIndex);
        if(it == mDependencyMap.end())
        {
        	Logger::error() << "Database index " << ref.dbIndex << " not found in level dependencies";
            throw NotFoundException("Can't get texture. Database index not found in level dependencies");
        }

        return it->second.get().getTexture(ref.assetId);
    }

    Model *Level::getModelByRef(const AssetRef &ref)
    {
    	Logger::debug() << "Requested model " << std::hex << ref.assetId << std::dec << " from level dependency " << ref.dbIndex;

        auto it = mDependencyMap.find(ref.dbIndex);
        if(it == mDependencyMap.end())
        {
        	Logger::error() << "Database index " << ref.dbIndex << " not found in level dependencies";
            throw NotFoundException("Can't get model. Database index not found in level dependencies");
        }

        return it->second.get().getModel(ref.assetId);
    }

    Class *Level::getClassByRef(const AssetRef &ref)
    {
        Logger::debug() << "Requested class " << std::hex << ref.assetId << std::dec << " from level dependency " << ref.dbIndex;

        auto it = mDependencyMap.find(ref.dbIndex);
        if(it == mDependencyMap.end())
        {
            Logger::error() << "Database index " << ref.dbIndex << " not found in level dependencies";
            throw NotFoundException("Can't get class. Database index not found in level dependencies");
        }

        return it->second.get().getClass(ref.assetId);
    }

    Sequence *Level::getSequenceByRef(const AssetRef &ref)
    {
        Logger::debug() << "Requested sequence " << std::hex << ref.assetId << std::dec << " from level dependency " << ref.dbIndex;

        auto it = mDependencyMap.find(ref.dbIndex);
        if(it == mDependencyMap.end())
        {
            Logger::error() << "Database index " << ref.dbIndex << " not found in level dependencies";
            throw NotFoundException("Can't get sequence. Database index not found in level dependencies");
        }

        return it->second.get().getSequence(ref.assetId);
    }

    Animation *Level::getAnimationByRef(const AssetRef &ref)
    {
    	Logger::debug() << "Requested animation " << std::hex << ref.assetId << std::dec << " from level dependency " << ref.dbIndex;

        auto it = mDependencyMap.find(ref.dbIndex);
        if(it == mDependencyMap.end())
        {
            Logger::error() << "Database index " << ref.dbIndex << " not found in level dependencies";
            throw NotFoundException("Can't get animation. Database index not found in level dependencies");
        }

        return it->second.get().getAnimation(ref.assetId);
    }

    Sound *Level::getSoundByRef(const AssetRef &ref)
    {
        Logger::debug() << "Requested sound " << std::hex << ref.assetId << std::dec << " from level dependency " << ref.dbIndex;

        auto it = mDependencyMap.find(ref.dbIndex);
        if(it == mDependencyMap.end())
        {
            Logger::error() << "Database index " << ref.dbIndex << " not found in level dependencies";
            throw NotFoundException("Can't get sound. Database index not found in level dependencies");
        }

        return it->second.get().getSound(ref.assetId);
    }

    void Level::_loadNameAndDeps(SrscFile &file)
    {
    	DataReader dr(file.getStreamForRecordType(SrscRecordType::LEVEL_NAME));

    	dr  >> mLevelName
            >> mMaxWidth
            >> mMaxHeight;

        uint32_t dbRefCount;
        dr >> dbRefCount;

        Logger::verbose() << "Level depends on " << dbRefCount << " databases";

        for(size_t i = 0; i < dbRefCount; ++i)
        {
        	uint16_t dbIndex;
            dr  >> dbIndex
            	>> DataReader::Expect<uint16_t>(0);

            std::string dbPathStr;
            dr >> dbPathStr;

            FilePath dbPath(dbPathStr, mLevelPath.dir());
            Database &db = mDbManager.loadDb(dbPath.adjustCase());

            Logger::debug() << "Level dependency index " << dbIndex << ": " << dbPath;

            mDependencyMap.insert(std::pair<uint16_t, DbRefWrapper>(dbIndex, db));
        }
    }

    void Level::_loadLayers(SrscFile &file)
    {
    	DataReader dr(file.getStreamForRecordType(SrscRecordType::LEVEL_LAYERS));

    	uint32_t layerCount;
    	dr >> layerCount;

    	mLayers.reserve(layerCount);

    	Logger::verbose() << "Level has " << layerCount << " layers";

    	for(size_t i = 0; i < layerCount; ++i)
    	{
    		osg::ref_ptr<Layer> layer(new Layer(*this));
    		layer->loadDefinition(dr);

    		mLayers.push_back(layer);
    	}

    	dr >> DataReader::Expect<uint32_t>(1);

    	for(size_t i = 0; i < layerCount; ++i)
    	{
    		uint32_t compressedDataSize;
			dr >> compressedDataSize;

			ZStream zstr(dr.getStream(), compressedDataSize, ZStreamBuffer::DefaultBufferSize);
			DataReader zdr(zstr);
			mLayers[i]->loadPolyData(zdr);
			zstr.seekToEndOfZlib();

			mLayers[i]->buildGeometry();
			mLayerGroup->addChild(mLayers[i]);

			if(mLayers[i]->getCollisionShape() != nullptr)
			{
				mPhysicsManager.addLayer(*mLayers[i]);
			}
    	}
    }

    void Level::_loadLayerGroups(SrscFile &file)
    {
    	DataReader dr(file.getStreamForRecordType(SrscRecordType::LEVEL_LAYERGROUPS));

    	uint32_t groupCount;
    	dr >> groupCount;

    	for(size_t i = 0; i < groupCount; ++i)
    	{
    		std::string groupName;
    		uint32_t layerCount;

    		dr >> groupName
			   >> layerCount;

    		Logger::info() << "Group " << groupName << " has " << layerCount << " layers:";

    		for(size_t j = 0; j < layerCount; ++j)
    		{
    			uint32_t layer;
    			dr >> layer;
    		}

    	}
    }

    void Level::_loadObjects(SrscFile &file)
    {
    	Logger::verbose() << "Loading level objects";

    	SrscFile::DirIterator objectRecord = file.getDirIteratorByType(SrscRecordType::LEVEL_OBJECTS);
    	if(objectRecord == file.getDirectoryEnd())
    	{
    		Logger::verbose() << "Level has no objects";
    		return; // if record does not appear level has no objects
    	}

    	DataReader dr(file.getStreamForRecord(objectRecord));

    	uint16_t objectCount;
    	dr >> objectCount;

    	Logger::verbose() << "Level has " << objectCount << " objects";

    	mLevelObjects.resize(objectCount);
    	for(size_t i = 0; i < objectCount; ++i)
    	{
    		osg::ref_ptr<od::LevelObject> object(new od::LevelObject(*this));
    		object->loadFromRecord(dr);

    		mLevelObjects[i] = object;
    	}
    }
}

