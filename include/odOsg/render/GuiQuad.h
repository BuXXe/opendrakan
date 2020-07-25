/*
 * GuiQuad.h
 *
 *  Created on: 14 Dec 2018
 *      Author: zal
 */

#ifndef INCLUDE_ODOSG_GUIQUAD_H_
#define INCLUDE_ODOSG_GUIQUAD_H_

#include <osg/Geometry>

#include <odCore/render/GuiQuad.h>

namespace odOsg
{
    class Texture;

    class GuiQuad : public odRender::GuiQuad
    {
    public:

        GuiQuad();
        virtual ~GuiQuad();

        inline osg::Geometry *getOsgGeometry() { return mGeometry; }

        virtual std::shared_ptr<odRender::Texture> getTexture() override;

        virtual void setTexture(std::shared_ptr<odRender::Texture> texture) override;
        virtual void setTextureCoords(const glm::vec2 &topLeft, const glm::vec2 &bottomRight) override;
        virtual void setTextureCoordsFromPixels(const glm::vec2 &topLeft, const glm::vec2 &bottomRight) override;
        virtual void setVertexCoords(const glm::vec2 &topLeft, const glm::vec2 &bottomRight) override;

        virtual void setColor(const glm::vec4 &color) override;


    private:

        osg::ref_ptr<osg::Geometry> mGeometry;
        osg::ref_ptr<osg::Vec3Array> mVertexArray;
        osg::ref_ptr<osg::Vec2Array> mTextureCoordArray;
        osg::ref_ptr<osg::Vec4Array> mColorArray;

        std::shared_ptr<Texture> mTexture;
    };

}

#endif /* INCLUDE_ODOSG_GUIQUAD_H_ */
