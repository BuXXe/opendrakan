/*
 * ContainerWidget.cpp
 *
 *  Created on: 15 Jul 2018
 *      Author: zal
 */

#include "gui/ContainerWidget.h"

#include <limits>

#include "gui/WidgetGroup.h"

namespace od
{

    ContainerWidget::ContainerWidget(GuiManager &gm)
    : Widget(gm)
    , mChildWidgetGroup(new WidgetGroup)
    {
        this->addChild(mChildWidgetGroup);
    }

    void ContainerWidget::addWidget(Widget *w)
    {
        mChildWidgetGroup->addChild(w);
        mChildWidgets.push_back(osg::ref_ptr<Widget>(w));

        w->setParent(this);
    }

}
