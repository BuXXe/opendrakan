/*
 * Engine.h
 *
 *  Created on: 14 Feb 2018
 *      Author: zal
 */

#ifndef INCLUDE_ENGINE_H_
#define INCLUDE_ENGINE_H_

#include <memory>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include "DbManager.h"
#include "ShaderManager.h"
#include "InputManager.h"
#include "light/LightManager.h"
#include "Level.h"

namespace od
{

    class GuiManager;
    class Player;
    class Camera;

	class Engine
	{
	public:

	    Engine();
		Engine(Engine &e) = delete;
		Engine(const Engine &e) = delete;
		~Engine() = default;

		inline const FilePath &getInitialLevelFile() const { return mInitialLevelFile; }
		inline void setInitialLevelFile(const FilePath &level) { mInitialLevelFile = level; }
		inline const FilePath &getEngineRootDir() const { return mEngineRootDir; }
		inline DbManager &getDbManager() { return mDbManager; }
		inline ShaderManager &getShaderManager() { return mShaderManager; }
		inline GuiManager &getGuiManager() { return *mGuiManager; }
		inline LightManager &getLightManager() { return *mLightManager; }
		inline Level &getLevel() { return *mLevel; } // FIXME: throw if no level present
		inline Player *getPlayer() { return mPlayer; }
        inline void setPlayer(Player *p) { mPlayer = p; }
		inline void setCamera(Camera *cam) { mCamera = cam; }
		inline Camera *getCamera() { return mCamera; }
		inline double getMaxFrameRate() const { return mMaxFrameRate; }
		inline void setMaxFrameRate(double fps) { mMaxFrameRate = fps; } // 0 for no cap

		void setUp();
		void run();


	private:

		void _findEngineRoot(const std::string &rrcFileName);

		DbManager mDbManager;
		ShaderManager mShaderManager;
		osg::ref_ptr<InputManager> mInputManager;
		std::unique_ptr<GuiManager> mGuiManager;
		std::unique_ptr<LightManager> mLightManager;
		FilePath mInitialLevelFile;
		FilePath mEngineRootDir;
		std::unique_ptr<Level> mLevel;
		osg::ref_ptr<osg::Group> mRootNode;
		osg::ref_ptr<osgViewer::Viewer> mViewer;
		osg::ref_ptr<osgViewer::ScreenCaptureHandler> mScreenshotHandler;
		Camera *mCamera;
		Player *mPlayer;
		double mMaxFrameRate;
		bool mSetUp;
	};

}

#endif /* INCLUDE_ENGINE_H_ */
