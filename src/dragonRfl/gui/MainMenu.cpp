/*
 * MainMenu.cpp
 *
 *  Created on: 29 Jun 2018
 *      Author: zal
 */

#include <dragonRfl/gui/MainMenu.h>

#include <dragonRfl/gui/GuiTextures.h>
#include <dragonRfl/gui/CrystalRingButton.h>
#include <dragonRfl/UserInterfaceProperties.h>
#include <odCore/Engine.h>
#include <odCore/db/Model.h>
#include <odCore/gui/GuiManager.h>
#include <odCore/gui/TexturedQuad.h>

namespace od
{


    class MainMenuImage : public Widget
    {
    public:

        MainMenuImage(GuiManager &gm)
        : Widget(gm)
        {
            osg::ref_ptr<TexturedQuad> topLeft = new TexturedQuad;
            topLeft->setTextureImage(gm.getTexture(GuiTextures::MainMenu_TopLeft));
            topLeft->setTextureCoordsFromPixels(osg::Vec2(0, 0), osg::Vec2(255, 255));
            topLeft->setVertexCoords(osg::Vec2(0.0, 0.0), osg::Vec2(0.5, 0.5));
            this->addDrawable(topLeft);

            osg::ref_ptr<TexturedQuad> topRight = new TexturedQuad;
            topRight->setTextureImage(gm.getTexture(GuiTextures::MainMenu_TopRight));
            topRight->setTextureCoordsFromPixels(osg::Vec2(0, 0), osg::Vec2(255, 255));
            topRight->setVertexCoords(osg::Vec2(0.5, 0.0), osg::Vec2(1, 0.5));
            this->addDrawable(topRight);

            osg::ref_ptr<TexturedQuad> bottomLeft = new TexturedQuad;
            bottomLeft->setTextureImage(gm.getTexture(GuiTextures::MainMenu_BottomLeft));
            bottomLeft->setTextureCoordsFromPixels(osg::Vec2(0, 0), osg::Vec2(255, 255));
            bottomLeft->setVertexCoords(osg::Vec2(0.0, 0.5), osg::Vec2(0.5, 1));
            this->addDrawable(bottomLeft);

            osg::ref_ptr<TexturedQuad> bottomRight = new TexturedQuad;
            bottomRight->setTextureImage(gm.getTexture(GuiTextures::MainMenu_BottomRight));
            bottomRight->setTextureCoordsFromPixels(osg::Vec2(0, 0), osg::Vec2(255, 255));
            bottomRight->setVertexCoords(osg::Vec2(0.5, 0.5), osg::Vec2(1.0, 1.0));
            this->addDrawable(bottomRight);

            this->setDimensions(512.0, 512.0, WidgetDimensionType::Pixels);
        }

    };

    class MainMenuBackground : public Widget
    {
    public:

        MainMenuBackground(GuiManager &gm)
        : Widget(gm)
        {
            osg::ref_ptr<TexturedQuad> bg = new TexturedQuad;
            bg->setVertexCoords(osg::Vec2(0.0, 0.0), osg::Vec2(1.0, 1.0));
            bg->setColor(osg::Vec4(0.0, 0.0, 0.0, 0.5));
            this->addDrawable(bg);

            this->setDimensions(1.0, 1.0, WidgetDimensionType::ParentRelative);
        }
    };



    MainMenu::MainMenu(GuiManager &gm, od::UserInterfaceProperties *uiProps)
    : ContainerWidget(gm)
    {
        osg::ref_ptr<MainMenuImage> imageWidget(new MainMenuImage(gm));
        imageWidget->setZIndex(0);
        imageWidget->setOrigin(WidgetOrigin::Center);
        imageWidget->setPosition(osg::Vec2(0.5, 0.5));
        this->addWidget(imageWidget);

        osg::ref_ptr<MainMenuBackground> bgWidget(new MainMenuBackground(gm));
        bgWidget->setZIndex(1);
        this->addWidget(bgWidget);

        // container for 3D elements
        osg::ref_ptr<ContainerWidget> cont(new ContainerWidget(gm));
        cont->setDimensions(512.0, 512.0, WidgetDimensionType::Pixels);
        cont->setPosition(0.5, 0.5);
        cont->setOrigin(WidgetOrigin::Center);
        this->addWidget(cont);

        osg::ref_ptr<osg::Light> light(new osg::Light(0));
        light->setAmbient(osg::Vec4(0.3, 0.3, 0.3, 1.0));
        light->setDiffuse(osg::Vec4(1.0, 1.0, 1.0, 1.0));
        light->setPosition(osg::Vec4(0.0, -1.0, -1.0, 0.0));
        light->setSpecular(osg::Vec4(1.0, 1.0, 1.0, 1.0));
        osg::ref_ptr<osg::LightSource> lightSource(new osg::LightSource);
        lightSource->setLight(light);
        cont->addChild(lightSource);

        cont->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

        _addCrystal(gm, uiProps->mCrystalTop.getAsset(), 53, 255, 62, uiProps, cont, BC_MULTIPLAYER);
        _addCrystal(gm, uiProps->mCrystalLeft.getAsset(), 57, 110, 193, uiProps, cont, BC_LOAD);
        _addCrystal(gm, uiProps->mCrystalMiddle.getAsset(), 67, 255, 191, uiProps, cont, BC_NEW);
        _addCrystal(gm, uiProps->mCrystalRight.getAsset(), 57, 400, 193, uiProps, cont, BC_SAVE);
        _addCrystal(gm, uiProps->mCrystalLowerLeft.getAsset(), 35, 152, 292, uiProps, cont, BC_OPTIONS);
        _addCrystal(gm, uiProps->mCrystalLowerRight.getAsset(), 35, 358, 292, uiProps, cont, BC_CREDITS);
        _addCrystal(gm, uiProps->mCrystalBottom.getAsset(), 61, 255, 440, uiProps, cont, BC_QUIT);

        this->setDimensions(1.0, 1.0, WidgetDimensionType::ParentRelative);
    }

    void MainMenu::_addCrystal(GuiManager &gm, Model *crystalModel, float dia, float x, float y,
            od::UserInterfaceProperties *uiProps, ContainerWidget *cont, int buttonCode)
    {
        osg::ref_ptr<CrystalRingButton> crystal(new CrystalRingButton(gm, crystalModel,
                uiProps->mInnerRing.getAsset(),
                uiProps->mOuterRing.getAsset()));
        crystal->setDimensions(dia, dia, WidgetDimensionType::Pixels);
        crystal->setPosition(x/512, y/512);
        crystal->setOrigin(WidgetOrigin::Center);
        crystal->setClickedCallback(std::bind(&MainMenu::_buttonClicked, this, std::placeholders::_1), buttonCode);
        cont->addWidget(crystal);
    }

    void MainMenu::_buttonClicked(int buttonCode)
    {
        switch(buttonCode)
        {
        default:
        case BC_MULTIPLAYER:
        case BC_LOAD:
        case BC_NEW:
        case BC_SAVE:
        case BC_OPTIONS:
        case BC_CREDITS:
            break;

        case BC_QUIT:
            Logger::info() << "Exit button clicked. Quitting game";
            getGuiManager().quit();
            break;
        }
    }

}
