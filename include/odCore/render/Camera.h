/*
 * Camera.h
 *
 *  Created on: Nov 29, 2018
 *      Author: zal
 */

#ifndef INCLUDE_ODCORE_RENDER_CAMERA_H_
#define INCLUDE_ODCORE_RENDER_CAMERA_H_

#include <glm/vec3.hpp>

namespace odRender
{

    class Camera
    {
    public:

        virtual ~Camera() = default;

        virtual glm::vec3 getEyePoint() = 0;

        /**
         * @brief Sets camera look direction using the same convention as gluLookAt
         */
        virtual void lookAt(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up) = 0;


    };

}


#endif /* INCLUDE_ODCORE_RENDER_CAMERA_H_ */
