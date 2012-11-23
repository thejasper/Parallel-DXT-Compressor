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
#include "SimpleDCTEnc.h"

SimpleDCTEnc::SimpleDCTEnc( int _quality ) : quality(_quality) {
	init();
}

SimpleDCTEnc::~SimpleDCTEnc(void) {}

void SimpleDCTEnc::init(void) {
	initQuantTables();
	initHuffmanTables();
}

void SimpleDCTEnc::initQuantTables(void) {
	// These are the sample quantization tables given in JPEG spec section K.1.
	// The spec says that the values given produce "good" quality, and
	// when divided by 2, "very good" quality.	

	static unsigned short std_luminance_quant_tbl[64] = 
	{
			16,  11,  10,  16,  24,  40,  51,  61,
			12,  12,  14,  19,  26,  58,  60,  55,
			14,  13,  16,  24,  40,  57,  69,  56,
			14,  17,  22,  29,  51,  87,  80,  62,
			18,  22,  37,  56,  68, 109, 103,  77,
			24,  35,  55,  64,  81, 104, 113,  92,
			49,  64,  78,  87, 103, 121, 120, 101,
			72,  92,  95,  98, 112, 100, 103,  99
	};
	static unsigned short std_chrominance_quant_tbl[64] = 
	{
			17,  18,  24,  47,  99,  99,  99,  99,
			18,  21,  26,  66,  99,  99,  99,  99,
			24,  26,  56,  99,  99,  99,  99,  99,
			47,  66,  99,  99,  99,  99,  99,  99,
			99,  99,  99,  99,  99,  99,  99,  99,
			99,  99,  99,  99,  99,  99,  99,  99,
			99,  99,  99,  99,  99,  99,  99,  99,
			99,  99,  99,  99,  99,  99,  99,  99
	};

	// Safety checking. Convert 0 to 1 to avoid zero divide. 
	scale = quality;

	if (scale <= 0) 
		scale = 1;
	if (scale > 100) 
		scale = 100;
	
	//	Non-linear map: 1->5000, 10->500, 25->200, 50->100, 75->50, 100->0
	if (scale < 50)
		scale = 5000 / scale;
	else
		scale = 200 - scale*2;

	//	Scale the Y and CbCr quant table, respectively
	scaleQuantTable(qtblY,    std_luminance_quant_tbl);
	scaleQuantTable(qtblCbCr, std_chrominance_quant_tbl);
}

void SimpleDCTEnc::scaleQuantTable(float* tblRst, unsigned short* tblStd) {
    
    static const double aanscalefactor[8] = {
        1.0, 1.387039845, 1.306562965, 1.175875602,
        1.0, 0.785694958, 0.541196100, 0.275899379
	};

	int i, temp, half = 1<<10;
	for (i = 0; i < 64; i++) {
		// (1) user scale up
		temp = (int)(( scale * tblStd[i] + 50 ) / 100 );

		// limit to baseline range 
		if (temp <= 0) 
			temp = 1;
		if (temp > 255)
			temp = 255;		

		// (2) scaling needed for AA&N algorithm
        int row = i/8;
        int col = i%8;
        tblRst[i] = (float)(1.0 / (temp * aanscalefactor[row] * aanscalefactor[col] * 8.0));
	}
}

void SimpleDCTEnc::initHuffmanTables(void) {

	//	Y dc component
	static unsigned char bitsYDC[17] =
    { 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
	static unsigned char valYDC[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

	//	CbCr dc
	static unsigned char bitsCbCrDC[17] =
    { 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
	static unsigned char valCbCrDC[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
	
	//	Y ac component
	static unsigned char bitsYAC[17] =
    { 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
	static unsigned char valYAC[] =
    { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa };

	//	CbCr ac
	static unsigned char bitsCbCrAC[17] =
    { 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
	static unsigned char valCbCrAC[] =
    { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa };

	//	Compute four derived Huffman tables
	computeHuffmanTable( bitsYDC, valYDC, &htblYDC );
	computeHuffmanTable( bitsYAC, valYAC, &htblYAC );

	computeHuffmanTable( bitsCbCrDC, valCbCrDC, &htblCoCgDC );
	computeHuffmanTable( bitsCbCrAC, valCbCrAC, &htblCoCgAC );
}

/*
*	Compute the derived values for a Huffman table.	
*
*	typedef struct {
*		unsigned int	code[256];	// code for each symbol 
*		char			size[256];	// length of code for each symbol 
*		//	If no code has been allocated for a symbol S, ehufsi[S] contains 0 
*	} HUFFMAN_TABLE;
*
*	HUFFMAN_TABLE m_htblYDC, m_htblYAC, m_htblCbCrDC, m_htblCbCrAC;
*/	
void SimpleDCTEnc::computeHuffmanTable(
		unsigned char *	pBits, 
		unsigned char * pVal,
		HUFFMAN_TABLE * pTbl	)
{
	int p, i, l, lastp, si;
	char huffsize[257];
	unsigned int huffcode[257];
	unsigned int code;
	
	/* Figure C.1: make table of Huffman code length for each symbol */
	/* Note that this is in code-length order. */
	
	p = 0;
	for (l = 1; l <= 16; l++) {
		for (i = 1; i <= (int) pBits[l]; i++)
			huffsize[p++] = (char) l;
	}
	huffsize[p] = 0;
	lastp = p;
	
	/* Figure C.2: generate the codes themselves */
	/* Note that this is in code-length order. */
	
	code = 0;
	si = huffsize[0];
	p = 0;
	while (huffsize[p]) {
		while (((int) huffsize[p]) == si) {
			huffcode[p++] = code;
			code++;
		}
		code <<= 1;
		si++;
	}
	
	/* Figure C.3: generate encoding tables */
	/* These are code and size indexed by symbol value */
	
	/* Set any codeless symbols to have code length 0;
	* this allows EmitBits to detect any attempt to emit such symbols.
	*/
	memset( pTbl->size, 0, sizeof( pTbl->size ) );
	
	for (p = 0; p < lastp; p++) {
		pTbl->code[ pVal[p] ] = huffcode[p];
		pTbl->size[ pVal[p] ] = huffsize[p];
	}
}

bool SimpleDCTEnc::compress(unsigned char *dest, const unsigned char *source, int width, int height, int& bytesWritten)
{
	//	Error handling
	if(( dest == 0 )||( source == 0 ))
		return false;

	this->width = width;
	this->height = height;
    bytesWritten = 0;
	
	int pitch = width*4;		

	if(( width <= 0 )||( pitch <= 0 )||( height <= 0 ))
		return false;

	//	declares
	int xPixel, yPixel, xTile, yTile, cxTile, cyTile, cxBlock, cyBlock;
	int x, y, nTrueRows, nTrueCols, nTileBytes;
	unsigned char byTile[1024];
    const unsigned char *pTileRow, *pLastPixel;
    unsigned char *pHolePixel;
		
	//	horizontal and vertical count of tile, macroblocks, 
	//	or MCU(Minimum Coded Unit), in 16*16 pixels
	cxTile = (width  + 15)>> 4;	
	cyTile = (height + 15)>> 4;

	//	horizontal and vertical count of block, in 8*8 pixels
	cxBlock = cxTile << 1;
	cyBlock = cyTile << 1;

	//	first set output bytes as zero
	nTileBytes = 0;

	//	four dc values set to zero, needed for compressing one new image
	dcY = dcCo = dcCg = dcA = 0;

	//	Initialize size (in bits) and value to be written out
	nPutBits = 0;
	nPutVal = 0;

	//	Run all the tiles, or macroblocks, or MCUs
	for( yTile = 0; yTile < cyTile; yTile++ ) {
		for( xTile = 0; xTile < cxTile; xTile++ ) {
			//	Get tile starting pixel position
			xPixel = xTile << 4;
			yPixel = yTile << 4;

			//	Get the true number of tile columns and rows
			nTrueRows = 16;
			nTrueCols = 16;			
			if( yPixel + nTrueRows > height )
				nTrueRows = height - yPixel;
			if( xPixel + nTrueCols > width )
				nTrueCols = width - xPixel;

			//	Prepare pointer to one row of this tile
			pTileRow = source + (yPixel-1) * pitch + xPixel * 4;

			//	Get tile data from pInBuf into byTile. If not full, padding 
			//	byTile with the bottom row and rightmost pixel
			for( y = 0; y < 16; y ++ )
			{
				if( y < nTrueRows )
				{	
					//	Get data of one row
					pTileRow += pitch;					
					memcpy( byTile + y * 16 * 4, pTileRow, nTrueCols * 4 );
					
					//	padding to full tile with the rightmost pixel
					if( nTrueCols < 16 )
					{
						pLastPixel = pTileRow + (nTrueCols - 1) * 4;
						pHolePixel = byTile + y * 16 * 4 + nTrueCols * 4;
						for( x = nTrueCols; x < 16; x ++ )
						{
							memcpy( pHolePixel, pLastPixel, 4 );
							pHolePixel += 4;
						}
					}
				}
				else
				{
					//	padding the hole rows with the bottom row
					memcpy( byTile + y * 16 * 4, 
							byTile + (nTrueRows - 1) * 16 * 4,
							16 * 4 );
				}
			}
		
			//	Compress one tile with the dct
            compressOneTile(byTile, dest + bytesWritten, nTileBytes);
			bytesWritten += nTileBytes;
		}
	}
	
	//	Maybe there are some bits left, send them here
	if( nPutBits > 0 ) {
		emitLeftBits(dest + bytesWritten, nTileBytes );
		bytesWritten += nTileBytes;
	}
	return true;
}

/*
    Compress one 16*16 pixel block with jpeg like DCT algorithm
*/
bool SimpleDCTEnc::compressOneTile(const unsigned char * source, unsigned char * dest, int& bytesWritten) {
	//	Four color components, 256 + 64 + 64 + 256 elements 
	int yCoCgA[640];

	//	The DCT outputs are returned scaled up by a factor of 8;
	//	they therefore have a range of +-8K for 8-bit source data 
	float coef[64];	
	int quantCoef[64];	

	//	Initialize to zero
	bytesWritten = 0;

	//	Color conversion and subsampling
	//	pY data is in block order, e.g. 
	//	block 0 is from pY[0] to pY[63], block 1 is from pY[64] to pY[127]
	RGBAToYCoCgA(source, yCoCgA);

	//	Do Y/Cb/Cr components, Y: 4 blocks; Cb: 1 block; Cr: 1 block; A: 4 blocks
	int i, nBytes;
	for( i=0; i<10; i++ ) {
		forwardDct(yCoCgA + i*64, coef);
		quantize(coef, quantCoef, i);		
		huffmanEncode(quantCoef, dest + bytesWritten, i, nBytes);
		bytesWritten += nBytes;
	}

    return true;
}

/*
*	Color converts one 16*16 tile from rgba to ycocga 4:2:0 for one tile
*	(2) Y has 4 blocks, with block 0 from pY[0] to pY[63], 
*		block 1 from pY[64] to pY[127], block 2 from pY[128] to pY[191], ...
*	(3) With Cb/Cr subsampling, i.e. 2*2 pixels get one Cb and one Cr
*		IJG use average for better performance; we just pick one from four
*	(4) Do unsigned->signed conversion, i.e. substract 128 
*/
void SimpleDCTEnc::RGBAToYCoCgA(const unsigned char * rgba, int * blocks) {
	int *py[4], *pco, *pcg, *pa[4];
    const unsigned char *pByte;

	pByte = rgba;

    for(int i=0; i<4; i++ ) {
		py[i] = blocks + 64*i;
    }
	pco	  = blocks + 256;
	pcg   = blocks + 320;
	pa[0] = blocks + 384;
	pa[1] = pa[0] + 64;
	pa[2] = pa[1] + 64;
	pa[3] = pa[2] + 64;

	for(int y=0; y<16; y++ ) {
		for(int  x=0; x<16; x++ ) {
			unsigned char r = *(pByte ++);
			unsigned char g = *(pByte ++);
			unsigned char b = *(pByte ++);
			unsigned char a = *(pByte ++);

			//	block number is ((y/8) * 2 + x/8): 0, 1, 2, 3
			*( py[((y>>3)<<1) + (x>>3)]++ ) = rgbToY(r,g,b) - 128;
			*( pa[((y>>3)<<1) + (x>>3)]++ ) = a-128;

            // 4:2:0 colors average 2x2 rectangle
			if( (!(y & 1L)) && (!(x & 1L)) ) {
                int avgs[3];

                for ( int i=0;i<3;i++) {
                   int temp = rgba[(y*16+x)*4+i]+rgba[(y*16+x+1)*4+i]+
                                rgba[((y+1)*16+x)*4+i]+rgba[((y+1)*16+x+1)*4+i];
                    avgs[i] = temp>>2;
                }

				*(pco++) = rgbToCo(avgs[0],avgs[1],avgs[2]);
				*(pcg++) = rgbToCg(avgs[0],avgs[1],avgs[2]);
			}
		}
	}
}

/************************************************************************** 
 * (1)	Direct dct algorithms:
 *	are also available, but they are much more complex and seem not to 
 *  be any faster when reduced to code.
 *
 *************************************************************************
 * (2)  LL&M dct algorithm:
 *	This implementation is based on an algorithm described in
 *  C. Loeffler, A. Ligtenberg and G. Moschytz, "Practical Fast 1-D DCT
 *  Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
 *  Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
 *	The primary algorithm described there uses 11 multiplies and 29 adds.
 *	We use their alternate method with 12 multiplies and 32 adds.
 *
 ***************************************************************************
 * (3)	AA&N DCT algorithm:
 * This implementation is based on Arai, Agui, and Nakajima's algorithm for
 * scaled DCT.  Their original paper (Trans. IEICE E-71(11):1095) is in
 * Japanese, but the algorithm is described in the Pennebaker & Mitchell
 * JPEG textbook (see REFERENCES section in file README).  The following 
 * code is based directly on figure 4-8 in P&M.
 *
 * The AA&N method needs only 5 multiplies and 29 adds. 
 *
 * The primary disadvantage of this method is that with fixed-point math,
 * accuracy is lost due to imprecise representation of the scaled
 * quantization values.  The smaller the quantization table entry, the less
 * precise the scaled value, so this implementation does worse with high-
 * quality-setting files than with low-quality ones.
 ***************************************************************************
 */

#define FIX_0_382683433  0.382683433f
#define FIX_0_541196100  0.541196100f
#define FIX_0_707106781  0.707106781f
#define FIX_1_306562965  1.306562965f

//	AA&N DCT algorithm implemention
void SimpleDCTEnc::forwardDct(int* data, float* coef) {
	static const int DCTSIZE = 8;
	int x, y;
	int *dataptr;
	float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
	float tmp10, tmp11, tmp12, tmp13;
	float z1, z2, z3, z4, z5, z11, z13;
    float *coefptr;
	
	/* Pass 1: process rows. */	
	dataptr = data;		//input
	coefptr = coef;		//output	
	for( y = 0; y < 8; y++ ) 
	{
		tmp0 = (float)(dataptr[0] + dataptr[7]);
		tmp7 = (float)(dataptr[0] - dataptr[7]);
		tmp1 = (float)(dataptr[1] + dataptr[6]);
		tmp6 = (float)(dataptr[1] - dataptr[6]);
		tmp2 = (float)(dataptr[2] + dataptr[5]);
		tmp5 = (float)(dataptr[2] - dataptr[5]);
		tmp3 = (float)(dataptr[3] + dataptr[4]);
		tmp4 = (float)(dataptr[3] - dataptr[4]);
		
		/* Even part */
		
		tmp10 = tmp0 + tmp3;	/* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		
		coefptr[0] = tmp10 + tmp11; /* phase 3 */
		coefptr[4] = tmp10 - tmp11;
		
		z1 = (tmp12 + tmp13) * FIX_0_707106781; /* c4 */
		coefptr[2] = tmp13 + z1;	/* phase 5 */
		coefptr[6] = tmp13 - z1;
		
		/* Odd part */
		
		tmp10 = tmp4 + tmp5;	/* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;
		
		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = (tmp10 - tmp12) * FIX_0_382683433;	/* c6 */
		z2 = (tmp10 * FIX_0_541196100) + z5;		/* c2-c6 */
		z4 = (tmp12 * FIX_1_306562965) + z5;		/* c2+c6 */
		z3 = tmp11 * FIX_0_707106781;			/* c4 */
		
		z11 = tmp7 + z3;		/* phase 5 */
		z13 = tmp7 - z3;
		
		coefptr[5] = z13 + z2;	/* phase 6 */
		coefptr[3] = z13 - z2;
		coefptr[1] = z11 + z4;
		coefptr[7] = z11 - z4;
		
		dataptr += 8;		/* advance pointer to next row */
		coefptr += 8;
	}
	
	/* Pass 2: process columns. */	

	coefptr = coef;		//both input and output
	for ( x = 0; x < 8; x++ ) 
	{
		tmp0 = coefptr[DCTSIZE*0] + coefptr[DCTSIZE*7];
		tmp7 = coefptr[DCTSIZE*0] - coefptr[DCTSIZE*7];
		tmp1 = coefptr[DCTSIZE*1] + coefptr[DCTSIZE*6];
		tmp6 = coefptr[DCTSIZE*1] - coefptr[DCTSIZE*6];
		tmp2 = coefptr[DCTSIZE*2] + coefptr[DCTSIZE*5];
		tmp5 = coefptr[DCTSIZE*2] - coefptr[DCTSIZE*5];
		tmp3 = coefptr[DCTSIZE*3] + coefptr[DCTSIZE*4];
		tmp4 = coefptr[DCTSIZE*3] - coefptr[DCTSIZE*4];
		
		/* Even part */
		
		tmp10 = tmp0 + tmp3;	/* phase 2 */
		tmp13 = tmp0 - tmp3;
		tmp11 = tmp1 + tmp2;
		tmp12 = tmp1 - tmp2;
		
		coefptr[DCTSIZE*0] = tmp10 + tmp11; /* phase 3 */
		coefptr[DCTSIZE*4] = tmp10 - tmp11;
		
		z1 = (tmp12 + tmp13) * FIX_0_707106781; /* c4 */
		coefptr[DCTSIZE*2] = tmp13 + z1; /* phase 5 */
		coefptr[DCTSIZE*6] = tmp13 - z1;
		
		/* Odd part */
		
		tmp10 = tmp4 + tmp5;	/* phase 2 */
		tmp11 = tmp5 + tmp6;
		tmp12 = tmp6 + tmp7;
		
		/* The rotator is modified from fig 4-8 to avoid extra negations. */
		z5 = (tmp10 - tmp12) * FIX_0_382683433; /* c6 */
		z2 = (tmp10 * FIX_0_541196100) + z5; /* c2-c6 */
		z4 = (tmp12 * FIX_1_306562965) + z5; /* c2+c6 */
		z3 = tmp11 * FIX_0_707106781; /* c4 */
		
		z11 = tmp7 + z3;		/* phase 5 */
		z13 = tmp7 - z3;
		
		coefptr[DCTSIZE*5] = z13 + z2; /* phase 6 */
		coefptr[DCTSIZE*3] = z13 - z2;
		coefptr[DCTSIZE*1] = z11 + z4;
		coefptr[DCTSIZE*7] = z11 - z4;
		
		coefptr++;			/* advance pointer to next column */
	}
}

void SimpleDCTEnc::quantize( float* coef, int *quantCoef, int iBlock /*block id; Y: 0,1,2,3; Cb: 4; Cr: 5; A: 6-10*/ ) {
	float temp;
	float qval, *pQuant;

	if( iBlock < 4 || iBlock > 5 )
		pQuant = qtblY;
	else
		pQuant = qtblCbCr;

	for (int i = 0; i < 64; i++) 
	{
		qval = pQuant[i];
		temp = coef[i];
        temp *= qval;

        /* Round to nearest integer.
        * Since C does not specify the direction of rounding for negative
        * quotients, we have to force the dividend positive for portability.
        * The maximum coefficient size is +-16K (for 12-bit data), so this
        * code should work for either 16-bit or 32-bit ints.
        */
	    quantCoef[i] = ((int) (temp + 16384.5f) - 16384);	
    }
}



////////////////////////////////////////////////////////////////////////////////

bool SimpleDCTEnc::huffmanEncode( 
		int* pCoef,				//	DCT coefficients
		unsigned char* pOut,	//	Output byte stream
		int iBlock,				//	0,1,2,3:Y; 4:Cb; 5:Cr; A: 6-10
		int& nBytes				//	Out, Byte number of Output stream
		)
{	
	/*
	* jpeg_natural_order[i] is the natural-order position of the i'th element
	* of zigzag order.
	*
	* When reading corrupted data, the Huffman decoders could attempt
	* to reference an entry beyond the end of this array (if the decoded
	* zero run length reaches past the end of the block).  To prevent
	* wild stores without adding an inner-loop test, we put some extra
	* "63"s after the real entries.  This will cause the extra coefficient
	* to be stored in location 63 of the block, not somewhere random.
	* The worst case would be a run-length of 15, which means we need 16
	* fake entries.
	*/
	static const int jpeg_natural_order[64+16] = {
			 0,  1,  8, 16,  9,  2,  3, 10,
			17, 24, 32, 25, 18, 11,  4,  5,
			12, 19, 26, 33, 40, 48, 41, 34,
			27, 20, 13,  6,  7, 14, 21, 28,
			35, 42, 49, 56, 57, 50, 43, 36,
			29, 22, 15, 23, 30, 37, 44, 51,
			58, 59, 52, 45, 38, 31, 39, 46,
			53, 60, 61, 54, 47, 55, 62, 63,
			63, 63, 63, 63, 63, 63, 63, 63,//extra entries for safety
			63, 63, 63, 63, 63, 63, 63, 63
	};
	
	int temp, temp2, nbits, k, r, i, nWrite;
	int *block = pCoef;
	int *pLastDc = &dcY;
	HUFFMAN_TABLE *dctbl, *actbl;

	nBytes = 0;

	if( iBlock < 4 || iBlock > 5 )
	{
		dctbl = & htblYDC;
		actbl = & htblYAC;
        if( iBlock < 4 ) {
            pLastDc = &dcY;
        } else {
            pLastDc = &dcA;
        }
	}
	else
	{
		dctbl = & htblCoCgDC;
		actbl = & htblCoCgAC;

		if( iBlock == 4 )
			pLastDc = &dcCo;
		else
			pLastDc = &dcCg;
	}
	
	/* Encode the DC coefficient difference per section F.1.2.1 */
	
	temp = temp2 = block[0] - (*pLastDc);
	*pLastDc = block[0];
	
	if (temp < 0) {
		temp = -temp;		/* temp is abs value of input */
		/* For a negative input, want temp2 = bitwise complement of abs(input) */
		/* This code assumes we are on a two's complement machine */
		temp2 --;
	}
	
	/* Find the number of bits needed for the magnitude of the coefficient */
	nbits = 0;
	while (temp) {
		nbits ++;
		temp >>= 1;
	}
	
	//	Write category number
	if (! emitBits( pOut + nBytes, dctbl->code[nbits], dctbl->size[nbits], nWrite ))
		return false;
	nBytes += nWrite;

	//	Write category offset
	if (nbits)			/* EmitBits rejects calls with size 0 */
	{
		if (! emitBits( pOut + nBytes, (unsigned int) temp2, nbits, nWrite ))
			return false;
		nBytes += nWrite;
	}
	
	////////////////////////////////////////////////////////////////////////////
	/* Encode the AC coefficients per section F.1.2.2 */
	
	r = 0;			/* r = run length of zeros */
	
	for (k = 1; k < 64; k++) 
	{
		if ((temp = block[jpeg_natural_order[k]]) == 0) 
		{
			r++;
		} 
		else 
		{
			/* if run length > 15, must emit special run-length-16 codes (0xF0) */
			while (r > 15) {
				if (! emitBits( pOut + nBytes, actbl->code[0xF0], 
								actbl->size[0xF0], nWrite ))
					return false;
				nBytes += nWrite;
				r -= 16;
			}
			
			temp2 = temp;
			if (temp < 0) {
				temp = -temp;		/* temp is abs value of input */
				/* This code assumes we are on a two's complement machine */
				temp2--;
			}
			
			/* Find the number of bits needed for the magnitude of the coefficient */
			nbits = 1;		/* there must be at least one 1 bit */
			while ((temp >>= 1))
				nbits++;
			
			/* Emit Huffman symbol for run length / number of bits */
			i = (r << 4) + nbits;
			if (! emitBits( pOut + nBytes, actbl->code[i], 
							actbl->size[i], nWrite ))
				return false;
			nBytes += nWrite;
			
			//	Write Category offset
			if (! emitBits( pOut + nBytes, (unsigned int) temp2, 
							nbits, nWrite ))
				return false;
			nBytes += nWrite;
			
			r = 0;
		}
	}
	
	//If all the left coefs were zero, emit an end-of-block code
	if (r > 0)
	{
		if (! emitBits( pOut + nBytes, actbl->code[0], 
						actbl->size[0], nWrite ))
			return false;
		nBytes += nWrite;
	}		
	
	return true;
}

/* Outputting bits to the output buffer */
/* Only the right 24 bits of put_buffer are used; the valid bits are
 * left-justified in this part.  At most 16 bits can be passed to EmitBits
 * in one call, and we never retain more than 7 bits in put_buffer
 * between calls, so 24 bits are sufficient.
 */

inline bool SimpleDCTEnc::emitBits(
		unsigned char* pOut,	//Output byte stream
		unsigned int code,		//Huffman code
		int size,				//Size in bits of the Huffman code
		int& nBytes				//Out, bytes length 
		)
{
	/* This routine is heavily used, so it's worth coding tightly. */
	int put_buffer = (int) code;
	int put_bits = nPutBits;
	
	nBytes = 0;
	
	/* if size is 0, caller used an invalid Huffman table entry */
	if (size == 0)
		return false;
	
	put_buffer &= (((int)1)<<size) - 1; /* mask off any extra bits in code */
	
	put_bits += size;					/* new number of bits in buffer */
	
	put_buffer <<= 24 - put_bits;		/* align incoming bits */
	
	put_buffer |= nPutVal;			/* and merge with old buffer contents */
	
	//	If there are more than 8 bits, write it out
	while (put_bits >= 8) 
	{
		//	Write one byte out !!!!
		*(pOut++) = (unsigned char) ((put_buffer >> 16) & 0xFF);
		nBytes ++;
/*	
		if (uc == 0xFF) {		//need to stuff a zero byte?
			*(pOut++) = (unsigned char) 0;	//	Write one byte out !!!!
			nBytes ++;
		}
*/
		put_buffer <<= 8;
		put_bits -= 8;
	}
	
	nPutVal	= put_buffer; /* update state variables */
	nPutBits	= put_bits;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

inline void SimpleDCTEnc::emitLeftBits(
		unsigned char* pOut,	//Output byte stream
		int& nBytes				//Out, bytes length 
		)
{
	
	unsigned char uc = (unsigned char) ((nPutVal >> 16) & 0xFF);
	*(pOut++) = uc;			//	Write one byte out !!!!
	nBytes = 1;

	nPutVal  = 0;
	nPutBits = 0;
}
