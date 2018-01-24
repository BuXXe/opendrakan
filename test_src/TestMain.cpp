/*
 * TestMain.cpp
 *
 *  Created on: 30 Dec 2017
 *      Author: zal
 */

#include <iostream>
#include <iomanip>
#include <memory>

#include "ZStream.h"
#include "SrscFile.h"
#include "DbManager.h"
#include "RiotDb.h"


void srscStat(od::SrscFile &file)
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

	auto it = file.getDirectoryBegin();
	while(it != file.getDirectoryEnd())
	{

		std::cout
			<< std::setw(6) << (it - file.getDirectoryBegin())
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

void printUsage()
{
	std::cout
		<< "Usage: opendrakan [options] <path to db file>" << std::endl
		<< "Options:" << std::endl
		<< "    -i <id>    Extract record with ID <id>" << std::endl
		<< std::endl;
}

int main(int argc, char **argv)
{
	if(argc <= 1)
	{
		printUsage();
		return 0;
	}

	std::string filename;
	bool extract = false;
	uint16_t extractRecordId = 0;
	for(int i = 1; i < argc; ++i)
	{
		std::string arg(argv[i]);

		if(arg == "-i")
		{
			if(i == argc - 1)
			{
				std::cout << "i flag needs ID argument" << std::endl;
				printUsage();
				return 1;
			}

			std::string arg2(argv[++i]);
			std::istringstream is(arg2);
			is >> std::hex >> extractRecordId;

		}else if(arg == "-x")
		{
		    extract = true;

		}else
		{
			filename = arg;
		}
	}

	if(filename.empty())
	{
	    std::cout << "Need at least a db file argument." << std::endl;
	    printUsage();
	    return 1;
	}


	try
	{
		/*od::DbManager dbm;

		std::cout << "Loading database " << filename << std::endl;

		od::RiotDb &db = dbm.loadDb(od::FilePath(filename));

		std::cout << "Successfully loaded database!" << std::endl;

		od::SrscFile &srscFile = db.getResourceContainer(od::ASSET_SOUND);

		if(extract)
		{
            if(extractRecordId > 0)
            {
                od::SrscFile::DirEntry dirEntry = srscFile.getDirectoryEntryByID(extractRecordId);
                srscFile.decompressRecord("out/", dirEntry, true);

                od::AssetPtr classTest = db.getAssetById(od::ASSET_SOUND, extractRecordId);

                std::cout << "The loaded asset has name: " << classTest->getName() << std::endl;

            }else
            {
                srscFile.decompressAll("out/", true);
            }

		}else
		{
		    srscStat(srscFile);
		}*/

		od::SrscFile srscFile(filename);

		/*if(extract)
		{
            if(extractRecordId > 0)
            {
                od::SrscFile::DirEntry dirEntry = srscFile.getDirectoryEntryByID(extractRecordId);
                srscFile.decompressRecord("out/", dirEntry, true);

                std::cout << "Extracting record " << std::hex << extractRecordId << std::dec << " to out/" << std::endl;

            }else
            {
                srscFile.decompressAll("out/", true);

                std::cout << "Extracting all records to out/" << std::endl;
            }

		}else
		{
		    srscStat(srscFile);
		}*/

		od::DataReader dr(srscFile.getStreamForRecordTypeId(0x0001, 0));

		dr.ignore(32);

		std::string layerName;
		dr >> layerName;

		dr.ignore(36);

		od::ZStream in(dr.getStream());
		std::ofstream out("out/layerout.dat", std::ios::out | std::ios::binary);
		size_t n = 0;

		int c = in.get();
		while(c != od::ZStream::traits_type::eof())
		{
		    c = in.get();
		    out.put(c);
		    n++;
		}

		std::cout << "Decompressed " << n << " bytes to out" << std::endl;

	}catch(std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
