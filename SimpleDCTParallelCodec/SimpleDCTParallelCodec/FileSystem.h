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
#pragma once
class FileSystem
{
public:
	
	static std::ifstream* OpenFile(std::string filename);
	static void CloseFile(std::ifstream* file);
	static size_t FileLength(std::string filename);
	//static size_t FileLength(std::ifstream* file);
	static void ReadFileToMemory(std::string filename, unsigned char** pBuffer, size_t& fileSize, size_t offset);

	static bool WriteMemoryToFile(std::string filename, unsigned char* pBuffer, size_t bufferLength);
	

};

