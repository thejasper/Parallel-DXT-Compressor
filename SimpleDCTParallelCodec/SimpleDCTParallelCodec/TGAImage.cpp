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
#include "TGAImage.h"
#include "FileSystem.h"



TGAImage::TGAImage(void)
{
}


TGAImage::~TGAImage(void)
{
}

void TGAImage::Write(int width, int height, unsigned char* pPayloadBGRA, std::string filename)
{
	int	bufferSize = width*height*3 + 18;
	int imgStart = 18;
	bool flipVertical = true;

	unsigned char* buffer = new unsigned char[bufferSize];
	memset( buffer, 0, 18 );
	// TGA header
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size
	
	// swap rgb to bgr
	for ( int i=imgStart, j=0 ; i<bufferSize ; i+=3, j+=4 ) 
	{
		buffer[i+0] = pPayloadBGRA[j+0];	// blue
		buffer[i+1] = pPayloadBGRA[j+1];	// green
		buffer[i+2] = pPayloadBGRA[j+2];	// red		
	}

	FileSystem::WriteMemoryToFile(filename, buffer, bufferSize );

	delete buffer;
}

bool TGAImage::Read(std::string filename, int& width, int& height, unsigned char** ppPayload)
{
	size_t bufferSize = FileSystem::FileLength(filename);
	unsigned char* buffer = new unsigned char[bufferSize];
	int imgStart = 18;
	
	FileSystem::ReadFileToMemory(filename, &buffer, bufferSize, 0);

	// Read information from the tga header
	
	int bpp = buffer[16];
	bool flipVertical = (buffer[17] >> 5) == 1;

	if (buffer[2] != 2 || (bpp != 24 && bpp != 32) || flipVertical)
	{
		// We need raw format and 24-bit or 32-bit pixels, unflipped
		return false;
	}
	
	width = buffer[13] << 8 | buffer[12];
	height = buffer[15] << 8 | buffer[14];	
	const int pitch = width*bpp/8;	
			 
	unsigned char* line = (flipVertical ? buffer + imgStart + (height-1) * pitch : buffer + imgStart);
	int step = (flipVertical ? -pitch : pitch);

	unsigned char* payload = new unsigned char[width * height * 4];
	unsigned char* dst = payload;
	
	if (bpp == 24)
	{
		for (int y=0;y<height;++y, line += step)
		{
			for ( int x=0; x<pitch; x+=3, dst+=4)
			{
				// swap rgb to bgr
				dst[0] = line[x+2]; 	// blue
				dst[1] = line[x+1];		// green
				dst[2] = line[x+0];		// red
				dst[3] = 255;			// alpha
			}
		}
	}
	else if (bpp == 32)
	{
		for (int y=0;y<height;++y, line += step)
		{
			for ( int x=0,j=0; x<pitch; x+=4, dst+=4)
			{
				// swap rgb to bgr
				dst[0] = line[x+2]; 	// blue
				dst[1] = line[x+1];	// green
				dst[2] = line[x+0];	// red
				dst[3] = line[x+3];	// alpha
			}
		}
	}

	*ppPayload = payload;

	delete buffer;

	return true;
}
