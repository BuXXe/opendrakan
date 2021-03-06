/*
 * Exception.cpp
 *
 *  Created on: 2 Feb 2018
 *      Author: zal
 */

#include "Exception.h"

namespace od
{

	Exception::Exception(const std::string &msg)
	: mMsg(msg)
	{
	}

	const char *Exception::what() const noexcept
	{
		return mMsg.c_str();
	}


	NotFoundException::NotFoundException(const std::string &msg, RecordType type, RecordId id)
	: Exception(msg)
	, mType(type)
	, mId(id)
	{
	}


	InvalidArgumentException::InvalidArgumentException(const std::string &msg)
	: Exception(msg)
	{
	}

	UnsupportedException::UnsupportedException(const std::string &msg)
	: Exception(msg)
	{
	}

	IoException::IoException(const std::string &msg)
	: Exception(msg)
	{
	}

}
