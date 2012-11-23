/**
 *
 *  This software module was originally developed for research purposes,
 *  by Multimedia Lab at Ghent University (Belgium).
 *  Its performance may not be optimized for specific applications.
 *
 *  Those intending to use this software module in hardware or software products
 *  are advized that its use may infringe existing patents. The developers of 
 *  this software module, their companies, Ghent Universtity, nor Multimedia Lab 
 *  have any liability for use of this software module or modifications thereof.
 *
 *  Ghent University and Multimedia Lab (Belgium) retain full right to modify and
 *  use the code for their own purpose, assign or donate the code to a third
 *  party, and to inhibit third parties from using the code for their products. 
 *
 *  This copyright notice must be included in all copies or derivative works.
 *
 *  For information on its use, applications and associated permission for use,
 *  please contact Prof. Rik Van de Walle (rik.vandewalle@ugent.be). 
 *
 *  Detailed information on the activities of
 *  Ghent University Multimedia Lab can be found at
 *  http://multimedialab.elis.ugent.be/.
 *
 *  Copyright (c) Ghent University 2004-2012.
 *
 **/
#include "stdafx.h"
#include "FileSystem.h"


/*
size_t FileSystem::FileLength(std::ifstream* file)
{
	std::ios::pos_type begin = file->tellg();
	file->seekg(0, std::ios::end);
	std::ios::pos_type end = file->tellg();
	return (size_t)(end - begin);
}*/

size_t FileSystem::FileLength(std::string filename)
{
	std::ifstream *file = OpenFile(filename);

	if (!file)
	{
		return 0;
	}

	std::ios::pos_type begin = file->tellg();
	file->seekg(0, std::ios::end);
	std::ios::pos_type end = file->tellg();
	size_t size = (size_t)(end - begin);
	CloseFile(file);
	return size;
}

std::ifstream* FileSystem::OpenFile(std::string filename)
{
	std::ifstream *result;
    result = new std::ifstream();
    
    result->open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
    if ( result->good() ) 
	{
		
        return result;
    }    

    delete result;
    return NULL;
}

void FileSystem::CloseFile(std::ifstream* file)
{
	file->close();
	delete file;
}

void FileSystem::ReadFileToMemory(std::string filename, unsigned char** pBuffer, size_t& fileSize, size_t offset)
{
	size_t fileLength = FileLength(filename);
	std::ifstream* file = OpenFile(filename);
	fileSize = fileLength - offset;	

	char* p = new char[fileSize];

	if (!p)
	{
		*pBuffer = NULL;
		return;
	}
	file->seekg(offset, std::ios::beg);
	file->read(p, fileSize);

	*pBuffer = (unsigned char*)p;

}

bool FileSystem::WriteMemoryToFile(std::string filename, unsigned char* pBuffer, size_t bufferLength)
{
	std::ofstream file;

	file.open(filename.c_str(), std::ios_base::out | std::ios_base::binary);
    if ( !file.good() ) 
	{
		return false;        
    }    

	file.write((char*)pBuffer, bufferLength);
	file.close();

	return true;
}