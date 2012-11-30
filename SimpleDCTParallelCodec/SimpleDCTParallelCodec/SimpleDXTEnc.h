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

#include <amp.h>

#define INSET_SHIFT 4
#define MSB5_MASK 0xF8 // the 5 most significant bits are 1
#define MSB6_MASK 0xFC // the 6 most significant bits are 1

class DXT1BlockEncode
{
public:
	static unsigned int getColorDistance(const unsigned int* first, const unsigned int* second) restrict(amp);

	static unsigned int encodeOneColor(const unsigned int* color) restrict(amp);
	static unsigned int encodeColors(const unsigned int* minColor, const unsigned int* maxColor) restrict(amp);

	static unsigned int calculateEndPoints(const unsigned int* block, unsigned int* minColor, unsigned int* maxColor) restrict(amp);
	static unsigned int calculateIndices(const unsigned int* block, const unsigned int* minColor, const unsigned int* maxColor) restrict(amp);
};

class SimpleDXTEnc
{
private:
	const int width, height;
	unsigned int* pDecompressedBGRA;
	unsigned char* pCompressedResult;

public:
	SimpleDXTEnc(unsigned char* img, int w, int h);
	~SimpleDXTEnc();

	void storeBits(unsigned int bits);
	void writeDDSHeader();

	bool compress(unsigned char* pDXTCompressed, int& compressedSize);
};
