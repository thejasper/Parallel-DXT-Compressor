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

#include <algorithm>
#include <cmath>

#include "stdafx.h"
#include "SimpleDXTEnc.h"

using namespace std;

SimpleDXTEnc::SimpleDXTEnc(unsigned char* img, int w, int h) : 
	pDecompressedBGRA(img), width(w), height(h)
{
}


SimpleDXTEnc::~SimpleDXTEnc()
{
}

unsigned int SimpleDXTEnc::getColorDistance(const unsigned char* first, const unsigned char* second)
{
	return pow(first[0]-second[0], 2) + pow(first[1]-second[1], 2) + pow(first[2]-second[2], 2);
}

void SimpleDXTEnc::transformBlock(const unsigned char* first, unsigned char* result)
{
	const int blockRowSize = 4 * blockSize;
	for (int i = 0; i < 4; ++i)
	{
		copy(first, first + blockRowSize, result + i*blockRowSize);
		first += width*4;
	}
}

void SimpleDXTEnc::calculateEndPoints(const unsigned char* block, unsigned char* minColor, unsigned char* maxColor)
{
	fill(minColor, minColor + 3, 255);
	fill(maxColor, maxColor + 3, 0);

	// iterate colors in block
	for (int i = 0; i < 16; ++i)
	{
		// iterate channels
		for (int c = 0; c < 3; ++ c)
		{
			// find the smallest and largest values for each channel
			if (block[i*4+c] < minColor[c])
				minColor[c] = block[i*4+c];
			if (block[i*4+c] > maxColor[c])
				maxColor[c] = block[i*4+c];
		}
	}

	// make RGB bounding box 1/16th of it's size smaller in every direction
	unsigned char inset[3];
	inset[0] = ( maxColor[0] - minColor[0] ) >> INSET_SHIFT;
	inset[1] = ( maxColor[1] - minColor[1] ) >> INSET_SHIFT;
	inset[2] = ( maxColor[2] - minColor[2] ) >> INSET_SHIFT;
	minColor[0] = (minColor[0] + inset[0] <= 255) ? minColor[0] + inset[0] : 255;
	minColor[1] = (minColor[1] + inset[1] <= 255) ? minColor[1] + inset[1] : 255;
	minColor[2] = (minColor[2] + inset[2] <= 255) ? minColor[2] + inset[2] : 255;
	maxColor[0] = (maxColor[0] >= inset[0]) ? maxColor[0] - inset[0] : 0;
	maxColor[1] = (maxColor[1] >= inset[1]) ? maxColor[1] - inset[1] : 0;
	maxColor[2] = (maxColor[2] >= inset[2]) ? maxColor[2] - inset[2] : 0;
}

unsigned int SimpleDXTEnc::calculateIndices(const unsigned char* block, const unsigned char* minColor, const unsigned char* maxColor)
{
	unsigned char colors[4][4];
	unsigned char indices[16];

	// save maximum, minimum and 2 interpolated colors for easy access
	colors[0][0] = (maxColor[0] & MSB5_MASK) | (maxColor[0] >> 5);
	colors[0][1] = (maxColor[1] & MSB6_MASK) | (maxColor[0] >> 6);
	colors[0][2] = (maxColor[2] & MSB5_MASK) | (maxColor[0] >> 5);
	colors[1][0] = (minColor[0] & MSB5_MASK) | (minColor[0] >> 5);
	colors[1][1] = (minColor[1] & MSB6_MASK) | (minColor[0] >> 6);
	colors[1][2] = (minColor[2] & MSB5_MASK) | (minColor[0] >> 5);
	colors[2][0] = (2 * maxColor[0] + 1 * minColor[0]) / 3;
	colors[2][1] = (2 * maxColor[1] + 1 * minColor[1]) / 3;
	colors[2][2] = (2 * maxColor[2] + 1 * minColor[2]) / 3;
	colors[3][0] = (1 * maxColor[0] + 2 * minColor[0]) / 3;
	colors[3][1] = (1 * maxColor[1] + 2 * minColor[1]) / 3;
	colors[3][2] = (1 * maxColor[2] + 2 * minColor[2]) / 3;

	for (int i = 0; i < 16; ++i)
	{
		unsigned int minDist = UINT_MAX;

		for (int j = 0; j < 4; ++j)
		{
			unsigned int dist = getColorDistance(block+i*4, &colors[j][0]);

			if (dist < minDist)
			{
				minDist = dist;
				indices[i] = j;
			}
		}
	}

	unsigned int result = 0;
	for (int i = 0; i < 16; ++i)
		result |= (indices[i] << (i * 2));
	
	return result;
}

unsigned short SimpleDXTEnc::encodeOneColor(const unsigned char* color)
{
	// encode one color in 5:6:5 format and make sure it's rgb, not bgr
	return ((color[2] >> 3) << 11) | ((color[1] >> 2) << 5) | (color[0] >> 3);
}
 
unsigned int SimpleDXTEnc::encodeColors(const unsigned char* minColor, const unsigned char* maxColor)
{
	unsigned int minBits = encodeOneColor(minColor);
	unsigned int maxBits = encodeOneColor(maxColor);

	// maxbits has to be greater, otherwise a 1-bit alpha channel is used (see specifications)
	if (minBits > maxBits)
		swap(minBits, maxBits);

	return minBits << 16 | maxBits;
}

void SimpleDXTEnc::storeBits(unsigned int bits)
{
	pCompressedResult[0] = (bits >> 0) & 255;
	pCompressedResult[1] = (bits >> 8) & 255;
	pCompressedResult[2] = (bits >> 16) & 255;
	pCompressedResult[3] = (bits >> 24) & 255;
	pCompressedResult += 4;
}

void SimpleDXTEnc::writeDDSHeader()
{
	// temporary hard coded header
	unsigned char data[] = {0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0xe0, 0x01, 0x00, 0x00, 0x80, 0x02, 0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x20, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	int n = sizeof(data) / sizeof(unsigned char);
	for (int i = 0; i < n; ++i)
		pCompressedResult[i] = data[i];
	pCompressedResult+=n;
}

bool SimpleDXTEnc::compress(unsigned char* pDXTCompressed, int& compressedSize)
{
	// check if the width and the height are a multiple of 4 (they should be)
	if (width % 4 || height % 4)
		return false;


	unsigned char block[4*blockSize*blockSize];
	unsigned char minColor[4];
	unsigned char maxColor[4];

	// iterate blocks
	unsigned char* pImg = pDecompressedBGRA;
	pCompressedResult = pDXTCompressed;
	
	writeDDSHeader();
	for (int r = 0; r < height / blockSize; ++r)
	{
		for (int c = 0; c < width / blockSize; ++c)
		{
			// save block in a linear array
			transformBlock(pImg + c*4*blockSize, block);

			calculateEndPoints(block, minColor, maxColor);

			storeBits(encodeColors(minColor, maxColor)); // store reference colors
			storeBits(calculateIndices(block, minColor, maxColor)); // store indices
		}

		// advance 4 lines
		pImg += width * 4 * 4;
	}

	compressedSize = pCompressedResult - pDXTCompressed;
	return true;
}