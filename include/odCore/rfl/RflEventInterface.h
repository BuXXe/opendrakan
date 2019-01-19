/*
 * RflEventInterface.h
 *
 *  Created on: Sep 20, 2018
 *      Author: zal
 */

#ifndef INCLUDE_ODCORE_RFL_RFLEVENTINTERFACE_H_
#define INCLUDE_ODCORE_RFL_RFLEVENTINTERFACE_H_


namespace odRfl
{

    class RflEventInterface
    {
    public:

        virtual ~RflEventInterface() = default;

        virtual void onStartup() = 0;


    };

}


#endif /* INCLUDE_ODCORE_RFL_RFLEVENTINTERFACE_H_ */
