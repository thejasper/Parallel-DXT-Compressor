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

#define INSET_SHIFT 4
#define MSB5_MASK 0xF8 // the 5 most significant bits are 1
#define MSB6_MASK 0xFC // the 6 most significant bits are 1

class SimpleDXTEnc
{
private:
	static const int blockSize = 4;

	const int width, height;
	unsigned char* pDecompressedBGRA, *pCompressedResult;

	unsigned int getColorDistance(const unsigned char* first, const unsigned char* second) const;
	void transformBlock(const unsigned char* first, unsigned char* result) const;

	void calculateEndPoints(const unsigned char* block, unsigned char* minColor, unsigned char* maxColor) const;
	unsigned int calculateIndices(const unsigned char* block, const unsigned char* minColor, const unsigned char* maxColor) const;

	unsigned short encodeOneColor(const unsigned char* color) const;
	unsigned int encodeColors(const unsigned char* minColor, const unsigned char* maxColor) const;

	void storeBits(unsigned int bits);
	void writeDDSHeader();

public:
	SimpleDXTEnc(unsigned char* img, int w, int h);
	~SimpleDXTEnc();

	bool compress(unsigned char* pDXTCompressed, int& compressedSize);
};
