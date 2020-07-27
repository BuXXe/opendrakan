/*
 * Cursor.cpp
 *
 *  Created on: 30 Jun 2018
 *      Author: zal
 */

#include <dragonRfl/gui/Cursor.h>

#include <odCore/render/Renderer.h>
#include <odCore/render/GuiNode.h>
#include <odCore/render/GuiQuad.h>
#include <odCore/render/Image.h>
#include <odCore/render/Texture.h>

#include <dragonRfl/gui/GuiTextures.h>
#include <dragonRfl/gui/DragonGui.h>

namespace dragonRfl
{

    Cursor::Cursor(DragonGui &gui)
    : Widget(gui)
    {
        std::shared_ptr<odRender::GuiQuad> cursorQuad = this->getRenderNode()->createGuiQuad();
        auto cursorDbTexture = gui.getAsset<odDb::Texture>(GuiTextures::Cursor);
        auto cursorImage = gui.getRenderer().getOrCreateImageFromDb(cursorDbTexture);
        auto texture = gui.getRenderer().createTexture(cursorImage, odRender::TextureReuseSlot::NONE);
        cursorQuad->setTexture(texture);

        // for some reason, the cursor image is offset left by 2 pixels with the pixels wrapping
        //  over to the right. maybe this is due to some strange way in which they implemented the
        //  cursor anchor? we fix this by using repeat mode for the U coordinate.
        texture->setEnableWrapping(odRender::Texture::Dimension::U, true);
        cursorQuad->setTextureCoordsFromPixels(glm::vec2(-2.5, 0), glm::vec2(29.5, 32));

        cursorQuad->setVertexCoords(glm::vec2(0.0, 0.0), glm::vec2(1, 1));

        this->setOrigin(odGui::WidgetOrigin::TopLeft);
        this->setDimensions(32.0, 32.0, odGui::WidgetDimensionType::Pixels);
    }

    bool Cursor::liesWithinLogicalArea(const glm::vec2 &pos)
    {
        // this causes the cursor widget to be excluded from any mouse events
        return false;
    }

}
