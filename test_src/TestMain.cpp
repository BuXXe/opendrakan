/*
 * TestMain.cpp
 *
 *  Created on: 30 Dec 2017
 *      Author: zal
 */

#include <iostream>
#include <iomanip>
#include <memory>
#include <unistd.h>

#include <osg/Group>
#include <osg/Geode>
#include <osgDB/ReadFile>
#include <osgGA/FirstPersonManipulator>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osgViewer/CompositeViewer>
#include <osgViewer/View>
#include <osgViewer/ViewerEventHandlers>

#include "ZStream.h"
#include "SrscFile.h"
#include "DbManager.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Database.h"
#include "Level.h"
#include "SrscRecordTypes.h"
#include "rfl/Rfl.h"


static void srscStat(od::SrscFile &file)
{
	std::cout << "Got SRSC version " << (int32_t)file.getVersion() << " with " << file.getRecordCount() << " records." << std::endl;
	std::cout << "Records:" << std::endl;

	std::cout << std::setw(6) << "Index"
			  << std::setw(6) << "Type"
			  << std::setw(8) << "Size"
			  << std::setw(6) << "RecID"
			  << std::setw(6) << "GruID"
			  << std::setw(24) << "Data"
			  << std::endl;

	const std::vector<od::SrscFile::DirEntry> &directory = file.getDirectory();
	auto it = directory.begin();
	while(it != directory.end())
	{

		std::cout
			<< std::setw(6) << (it - directory.begin())
			<< std::setw(6) << std::hex << it->type << std::dec
			<< std::setw(8) << it->dataSize
			<< std::setw(6) << std::hex << it->recordId << std::dec
			<< std::setw(6) << std::hex << it->groupId << std::dec;

		std::cout << "  ";

		od::DataReader dr(file.getStreamForRecord(*it));

		for(size_t i = 0; i < it->dataSize; ++i)
		{
			uint8_t b;
			dr >> b;

			printf("%02x ", b); // whatcha gonna do bout it?

			if(i >= 16)
			{
				std::cout << " [...]";
				break;
			}
		}

		std::cout << std::endl;

		++it;
	}
}

static void statClasses(od::SrscFile &file)
{
	for(auto it = file.getDirIteratorByType(OD_SRSC_CLASS); it != file.getDirectoryEnd(); it = file.getDirIteratorByType(OD_SRSC_CLASS, it+1))
	{
		od::DataReader dr(file.getStreamForRecord(*it));

		std::string name;
		uint16_t dummy;
        od::AssetRef modelRef;
        uint16_t classType;
        uint16_t iconNumber;
        uint32_t fieldCount;
        uint32_t dwordCount;

        dr >> name
		   >> dummy
		   >> modelRef
		   >> classType
		   >> iconNumber
		   >> fieldCount
		   >> dwordCount;

        dr.ignore(4*dwordCount);

        std::cout
				<< "[" << name << "]" << std::endl
        		<< "Dummy: " << std::hex << dummy << std::dec << std::endl
				<< "Model: " << modelRef << std::endl
				<< "RflClass: " << std::hex << classType << std::dec << std::endl
				<< "Icon: " << std::hex << iconNumber << std::dec << std::endl
				<< "Fields: " << fieldCount << std::endl;

        for(size_t i = 0; i < fieldCount; ++i)
        {
        	std::cout << "    ";

        	uint32_t type;
        	std::string name;
        	dr >> type
			   >> name;

        	switch(type & 0xff)
        	{
        	case od::RflField::INTEGER:
        		std::cout << "int32";
        		break;

            case od::RflField::FLOAT:
            	std::cout << "float";
        		break;

            case od::RflField::CLASS:
            	std::cout << "class";
        		break;

            case od::RflField::MODEL:
            	std::cout << "model";
        		break;

            case od::RflField::SOUND:
            	std::cout << "sound";
        		break;

            case od::RflField::ENUM:
            	std::cout << "enum";
        		break;

            case od::RflField::CHAR_CHANNEL:
            	std::cout << "channel";
        		break;

            case od::RflField::ANIMATION:
            	std::cout << "anim";
        		break;

            case od::RflField::STRING:
            	std::cout << "string";
        		break;

            case od::RflField::SEUQUENCE:
            	std::cout << "sequence";
        		break;

            case od::RflField::TEXTURE:
            	std::cout << "texture";
        		break;

            case od::RflField::COLOR:
            	std::cout << "color";
        		break;

            default:
            	std::cout << std::hex << (type & 0xff) << std::dec;
        	}

        	if(type & 0x1000)
        	{
        		std::cout << "[]";
        	}

        	std::cout << "\t\t" << name << std::endl;
        }

        std::cout << std::endl;
	}

}

void printUsage()
{
	std::cout
		<< "Usage: opendrakan [options] <file>" << std::endl
		<< "Options:" << std::endl
		<< "    -o <path>  Output path (default './out/')" << std::endl
		<< "    -x         Extract raw records" << std::endl
		<< "    -t         Extract as a texture" << std::endl
		<< "    -i <id>    Limit extraction to records with ID <id>" << std::endl
		<< "    -s         Print SRSC statistics" << std::endl
		<< "    -c         Create class statistics" << std::endl
		<< "    -v         Increase verbosity of logger" << std::endl
		<< "If no option is given, the file is loaded as a level." << std::endl
		<< std::endl;
}

int main(int argc, char **argv)
{
	if(argc <= 1)
	{
		printUsage();
		return 0;
	}

	od::Logger::LogLevel logLevel = od::Logger::LOGLEVEL_INFO;
	std::string filename;
	std::string outputPath = "out/";
	bool extract = false;
	bool texture = false;
	bool stat = false;
	bool classStat = false;
	uint16_t extractRecordId = 0;
	int c;
	while((c = getopt(argc, argv, "i:o:txscv")) != -1)
	{
		switch(c)
		{
		case 'i':
			{
				std::istringstream iss(optarg);
				iss >> std::hex >> extractRecordId;
				if(iss.fail())
				{
					std::cout << "Argument to -i must be a hex ID number" << std::endl;
					return 1;
				}
			}
			break;

		case 'x':
			extract = true;
			break;

		case 's':
			stat = true;
			break;

		case 'c':
			classStat = true;
			break;

		case 'v':
			if(logLevel < od::Logger::LOGLEVEL_DEBUG)
			{
				logLevel = static_cast<od::Logger::LogLevel>(1 + static_cast<int>(logLevel)); // i know, yucky enum abuse
			}
			break;

		case 't':
			texture = true;
			break;

		case 'o':
			outputPath = std::string(optarg);
			break;

		case '?':
			if(optopt == 'i')
			{
				std::cerr << "Option -i requires a hex ID argument." << std::endl;

			}else if(optopt == 'o')
			{
				std::cerr << "Option -o requires a valid path argument" << std::endl;

			}else
			{
				std::cout << "Unknown option -" << optopt << std::endl;
				printUsage();
			}
			return 1;
		}
	}

	if(optind >= argc)
	{
		std::cout << "Need at least a file argument." << std::endl;
	    printUsage();
	    return 1;
	}

	filename = std::string(argv[optind]);

	try
	{
		od::Logger::getDefaultLogger().setOutputLogLevel(logLevel);

		od::Rfl &rfl = od::Rfl::getSingleton();
		od::Logger::info() << "Got RFL " << rfl.getName() << " with " << rfl.getClassTypeCount() << " registered class types";

		if(stat)
		{
		    od::SrscFile srscFile(filename);

		    srscStat(srscFile);

		}else if(extract)
        {
			od::SrscFile srscFile(filename);

			if(extractRecordId > 0)
			{
				std::cout << "Extracting all records with ID " << std::hex << extractRecordId << std::dec << " to " << outputPath << std::endl;

				od::SrscFile::DirIterator dirIt = srscFile.getDirIteratorById(extractRecordId);
				while(dirIt != srscFile.getDirectoryEnd())
				{
					srscFile.decompressRecord(outputPath, dirIt, true);

					dirIt = srscFile.getDirIteratorById(extractRecordId, dirIt);
				}

			}else
			{
				srscFile.decompressAll(outputPath, true);

				std::cout << "Extracting all records to " << outputPath << std::endl;
			}

        }else if(texture)
        {
        	od::DbManager dbm;
        	od::Database &db = dbm.loadDb(filename);

        	od::AssetRef ref;
        	ref.assetId = extractRecordId;
        	ref.dbIndex = 0;

        	od::TexturePtr tex = db.getTextureByRef(ref);

			std::ostringstream ss;
			ss << outputPath << "texture" << extractRecordId << ".png";

        	tex->exportToPng(ss.str());

		}else if(classStat)
        {

			od::SrscFile srscFile(filename);

		    statClasses(srscFile);

        }else
		{
		    od::DbManager dbm;
		    osg::ref_ptr<od::Level> level(new od::Level(filename, dbm));

		    osg::ref_ptr<osg::Group> rootNode(new osg::Group);
		    rootNode->addChild(level);

		    osgViewer::CompositeViewer composite;
		    osg::ref_ptr<osgViewer::Viewer> viewer(new osgViewer::Viewer);
		    viewer->getCamera()->setClearColor(osg::Vec4(0.2,0.2,0.2,1));
		    viewer->setSceneData(rootNode);
		    composite.addView(viewer);

		    osg::ref_ptr<osgViewer::View> view(new osgViewer::View);
		    osg::ref_ptr<osgViewer::StatsHandler> statsHandler(new osgViewer::StatsHandler);
		    statsHandler->setKeyEventPrintsOutStats('s');
		    viewer->addEventHandler(statsHandler);
		    composite.addView(view);

		    return viewer->run();
		}


	}catch(std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
