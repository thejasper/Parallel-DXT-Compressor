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

#include <algorithm>
#include <cmath>

#include "SimpleDXTEnc.h"

using namespace std;
using namespace concurrency;

SimpleDXTEnc::SimpleDXTEnc(unsigned char* img, int w, int h) : 
	width(w), height(h)
{
	pDecompressedBGRA = new unsigned int[width * height];

	// transform each pixel consisting of 4 unsigned chars to one unsigned integer
	for (int i = 0; i < w * h; ++i, img += 4)
		pDecompressedBGRA[i] = img[0] << 24 | img[1] << 16 | img[2] << 8 | img[3];
}


SimpleDXTEnc::~SimpleDXTEnc()
{
	delete[] pDecompressedBGRA;
}

unsigned int SimpleDXTEnc::getColorDistance(const unsigned char* first, const unsigned char* second) const
{
	// calculate euclidian distance between colors
	return pow(first[0]-second[0], 2) + pow(first[1]-second[1], 2) + pow(first[2]-second[2], 2);
}

void SimpleDXTEnc::calculateEndPoints(const unsigned char* block, unsigned char* minColor, unsigned char* maxColor) const
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

unsigned int SimpleDXTEnc::calculateIndices(const unsigned char* block, const unsigned char* minColor, const unsigned char* maxColor) const
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

	// for each block: save the index to the color that is the best match
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

	// pack in a 32-bit integer
	unsigned int result = 0;
	for (int i = 0; i < 16; ++i)
		result |= (indices[i] << (i * 2));
	return result;
}

unsigned short SimpleDXTEnc::encodeOneColor(const unsigned char* color) const
{
	// encode one color in 5:6:5 format and make sure it's rgb, not bgr
	return ((color[2] >> 3) << 11) | ((color[1] >> 2) << 5) | (color[0] >> 3);
}
 
unsigned int SimpleDXTEnc::encodeColors(const unsigned char* minColor, const unsigned char* maxColor) const
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

// DDS specifications: http://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
void SimpleDXTEnc::writeDDSHeader()
{
	unsigned int code[] = {'D', 'X', 'T', '1'};

	storeBits(0x20534444);					// magic value
	storeBits(124);							// size of the header
	storeBits(0x1 | 0x2 | 0x4 | 0x1000);	// valid fields
	storeBits(height);						// height
	storeBits(width);						// width
	storeBits(0);							// dwPitchOrLinearSize
	storeBits(0);							// dwDepth
	storeBits(0);							// dwMipMapCount
	for (int i = 0; i < 11; ++i)			// dwReserved1[11]
		storeBits(0);						
	storeBits(32);							// size of the pixelformat header
	storeBits(0x4);							// dwFlags, texture contains compressed rgb data
	storeBits(code[0] | (code[1] << 8) |    // dxt1 format
		(code[2] << 16) | (code[3] << 24));
	storeBits(16);							// dwRGBBitCount
	storeBits(0);							// dwRBitMask
	storeBits(0);							// dwGBitMask
	storeBits(0);							// dwBBitMask
	storeBits(0);							// dwABitMask
	storeBits(0x1000);						// dwCaps
	storeBits(0);							// dwCaps2
	storeBits(0);							// dwCaps3
	storeBits(0);							// dwCaps4
	storeBits(0);							// dwReserved2
}

bool SimpleDXTEnc::compress(unsigned char* pDXTCompressed, int& compressedSize)
{
	// check if the width and the height are a multiple of 4 (they should be)
	if (width % 4 || height % 4)
		return false;

	compressedSize = (width * height) / 4;
	unsigned int* result = new unsigned int[compressedSize];

	array_view<const unsigned int, 2> uncompressedBGRA(height, width, pDecompressedBGRA); // input
	array_view<unsigned int, 1> compressedDXT(compressedSize, result); // output

	parallel_for_each(
		uncompressedBGRA.extent.tile<4, 4>(), 
		[=](tiled_index<4, 4> idx) restrict(amp)
		{
			// pixels in the block, row by row
			tile_static unsigned int block[4*4];

			// fill the tile and wait till it's completed
			block[idx.local[0]*4 + idx.local[1]] = uncompressedBGRA[idx.global];
			idx.barrier.wait();

			// execute remainer only once
			if (idx.local[0] != 0 || idx.local[1] != 0)
				return;

			// calculate end points

			// calculate optimal indices to one of endpoints or interpolated values

			// store the result (8 bytes)

		}
	);

	return true;
}