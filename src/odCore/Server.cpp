/*
 * Server.cpp
 *
 *  Created on: Sep 26, 2019
 *      Author: zal
 */

#include <odCore/Server.h>

#include <thread>
#include <chrono>

#include <odCore/Level.h>

#include <odCore/net/ServerConnector.h>

#include <odCore/physics/bullet/BulletPhysicsSystem.h>

#include <odCore/rfl/Rfl.h>
#include <odCore/rfl/RflManager.h>

#include <odCore/state/StateManager.h>

#include <odCore/input/InputManager.h>

namespace od
{

    class LocalServerConnector : public odNet::ServerConnector
    {
    public:

        LocalServerConnector(Server &server, odNet::ClientId client)
        : mServer(server)
        , mClientId(client)
        {
        }

        virtual void actionTriggered(odInput::ActionCode code, odInput::ActionState state) override
        {
            mServer.getInputManagerForClient(mClientId).injectAction(code, state);
        }

        virtual void analogActionTriggered(odInput::ActionCode code, const glm::vec2 &axes) override
        {
            mServer.getInputManagerForClient(mClientId).injectAnalogAction(code, axes);
        }


    private:

        Server &mServer;
        odNet::ClientId mClientId;

    };


    Server::Server(odDb::DbManager &dbManager, odRfl::RflManager &rflManager)
    : mDbManager(dbManager)
    , mRflManager(rflManager)
    , mIsDone(false)
    , mNextClientId(1)
    {
        mPhysicsSystem = std::make_unique<odBulletPhysics::BulletPhysicsSystem>(nullptr);
    }

    Server::~Server()
    {
    }

    std::unique_ptr<odNet::ServerConnector> Server::createLocalConnector(odNet::ClientId clientId)
    {
        return std::make_unique<LocalServerConnector>(*this, clientId);
    }

    odNet::ClientId Server::addClientConnector(std::unique_ptr<odNet::ClientConnector> connector)
    {
        odNet::ClientId newClientId = mNextClientId++;

        Client client;
        client.connector = std::move(connector);
        client.inputManager = std::make_unique<odInput::InputManager>();

        mClients.insert(std::make_pair(newClientId, std::move(client)));

        return newClientId;
    }

    odInput::InputManager &Server::getInputManagerForClient(odNet::ClientId id)
    {
        auto it = mClients.find(id);
        if(it == mClients.end())
        {
            throw od::NotFoundException("Invalid client ID");
        }

        return *(it->second.inputManager);
    }

    void Server::loadLevel(const FilePath &lvlPath)
    {
        Logger::verbose() << "Server loading level " << lvlPath;

        Engine engine(*this);

        mLevel = std::make_unique<Level>(engine);
        mLevel->loadLevel(lvlPath.adjustCase(), mDbManager);

        mStateManager = std::make_unique<odState::StateManager>(*mLevel);

        mLevel->spawnAllObjects();

        // in order for clients to be able to load the level, we have to give them
        //  a level path that does not depend on the engine root. for levels that are
        //  stored under the engine root, we can just trim off that part of the path.
        //  for out-of-tree-levels (not under engine root) we have no way to tell the
        //  client where they might find that particular file on their system, so we
        //  fall back to providing the full path, which will work fine for local clients.
        //  networked clients will probably not be used with out-of-tree-levels, anyway.
        auto relLevelPath = lvlPath.removePrefix(getEngineRootDir()).str();

        for(auto &client : mClients)
        {
            client.second.connector->loadLevel(relLevelPath);
        }

        mRflManager.forEachLoadedRfl([this](odRfl::Rfl &rfl){ rfl.onLevelLoaded(*this); });
    }

    void Server::run()
    {
        Logger::info() << "OpenDrakan server starting...";

        Logger::info() << "Server set up. Starting main server loop";

        double targetUpdateIntervalNs = (1e9/60.0);
        auto targetUpdateInterval = std::chrono::nanoseconds((int64_t)targetUpdateIntervalNs);
        auto lastUpdateStartTime = std::chrono::high_resolution_clock::now();
        double serverTime = 0.0;
        while(!mIsDone.load(std::memory_order_relaxed))
        {
            auto loopStart = std::chrono::high_resolution_clock::now();

            double relTime = 1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(loopStart - lastUpdateStartTime).count();
            lastUpdateStartTime = loopStart;

            serverTime += relTime;

            if(mLevel != nullptr)
            {
                mLevel->update(relTime);
            }

            mPhysicsSystem->update(relTime);

            for(auto &client : mClients)
            {
                client.second.inputManager->update(relTime);
            }

            // commit update
            odState::TickNumber latestTick = mStateManager->getLatestTick();
            mStateManager->commit(serverTime);
            for(auto &client : mClients)
            {
                // for now, send every tick. later, we'd likely adapt the rate with which we send snapshots based on the client's network speed
                mStateManager->sendSnapshotToClient(latestTick, *client.second.connector, client.second.lastSentTick);
                client.second.lastSentTick = latestTick;
            }

            auto loopEnd = std::chrono::high_resolution_clock::now();
            auto loopTime = loopEnd - loopStart;
            if(loopTime < targetUpdateInterval)
            {
                std::this_thread::sleep_for(targetUpdateInterval - loopTime);

            }else
            {
                float loopTimeMs = 1e-3 * std::chrono::duration_cast<std::chrono::microseconds>(loopTime).count();
                Logger::warn() << "Server tick took too long (" << loopTimeMs << "ms, target was " << (targetUpdateIntervalNs*1e-6) << "ms)";
            }
        }

        Logger::info() << "Shutting down server gracefully";
    }

    Server::Client::Client()
    : lastSentTick(odState::StateManager::FIRST_TICK - 1)
    {
    }

}
