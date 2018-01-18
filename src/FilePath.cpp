/*
 * FilePath.cpp
 *
 *  Created on: 9 Jan 2018
 *      Author: zal
 */

#include "FilePath.h"

#include <sstream>

#include "StringUtils.h"

#if defined (__WIN32__)
#	define OD_FILEPATH_SEPERATOR	    '\\'
#else
#	define OD_FILEPATH_SEPERATOR	    '/'
#endif

namespace od
{

	FilePath::FilePath(const std::string &path)
	{
	    _parsePath(path);
	}

	FilePath::FilePath(const std::string &path, FilePath relativeTo)
	{
	    _parsePath(relativeTo.str() + OD_FILEPATH_SEPERATOR + path);
	}

	FilePath FilePath::dir() const
	{
	    size_t lastSlash = mGoodPath.find_last_of("/\\");
	    std::string newPath = mGoodPath.substr(0, lastSlash);
	    return FilePath(newPath);
	}

	std::string FilePath::str() const
	{
	    std::string hostPath = mGoodPath;

	    for(size_t i = 0; i < hostPath.size(); ++i)
        {
            if((hostPath[i] == '\\' || hostPath[i] == '/') && hostPath[i] != OD_FILEPATH_SEPERATOR)
            {
                hostPath[i] = OD_FILEPATH_SEPERATOR;
            }
        }

	    return mGoodPath;
	}

	bool FilePath::operator==(const FilePath &right) const
    {
	    return mGoodPath == right.mGoodPath;
    }

	void FilePath::_parsePath(const std::string &path)
	{
		mGoodPath = path;
		for(size_t i = 0; i < mGoodPath.size(); ++i)
		{
		    if(mGoodPath[i] == '\\')
		    {
		        mGoodPath[i] = '/';
		    }
		}
	}
}





