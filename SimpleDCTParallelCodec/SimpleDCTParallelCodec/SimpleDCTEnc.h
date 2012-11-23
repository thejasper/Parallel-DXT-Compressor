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


/* Based on code by Tony Lin, which is based on code by the IJG 
    http://www.codeproject.com/KB/graphics/tonyjpeglib.aspx
*/
class SimpleDCTEnc {

	//	Derived data constructed for each Huffman table 	
	struct HUFFMAN_TABLE {
		unsigned int	code[256];	// code for each symbol 
		char			size[256];	// length of code for each symbol 
		//If no code has been allocated for a symbol S, size[S] is 0 
	};

	unsigned short quality, scale;
	float qtblY[64], qtblCbCr[64];

	HUFFMAN_TABLE htblYDC;
    HUFFMAN_TABLE htblYAC;
    HUFFMAN_TABLE htblCoCgDC;
    HUFFMAN_TABLE htblCoCgAC;
    // For alpha we just use the Y huff table for now...

	unsigned short width, height;

	// The dc records used for PCM of the dc's
	int dcY, dcCo, dcCg, dcA;

	// The size (in bits) and value (in 4 byte buffer) to be written out
	int nPutBits, nPutVal;
	
	void init(void);
	void initQuantTables(void);
	void scaleQuantTable(float* tblRst, unsigned short* tblStd);
	void initHuffmanTables( void );
	void computeHuffmanTable(unsigned char *	pBits, unsigned char * pVal, HUFFMAN_TABLE * pTbl);


	bool compressOneTile(const unsigned char * source, unsigned char * dest, int& bytesWritten);
    void RGBAToYCoCgA(const unsigned char * rgba, int * blocks);
	void forwardDct(int* data, float* coef);	
	void quantize(float* coef, int *quantCoef, int iBlock);
	bool huffmanEncode(int* pCoef,unsigned char* pOut,int iBlock,int& nBytes);
	bool emitBits(unsigned char* pOut,unsigned int code,int size,int& nBytes);
	void emitLeftBits(unsigned char* pOut,int& nBytes);

public:	
	SimpleDCTEnc( int nQuality=50 );
	~SimpleDCTEnc();

    // Compresses the pixel data it stores raw data so quanization parameter, width, height will have to be stored by the caller
	bool compress(unsigned char *dest, const unsigned char *source, int width, int weight, int& bytesWritten);
};

inline int rgbToY(int r, int g, int b) {
    return (r + (g<<1) + b)  >> 2;
}

inline int rgbToCo(int r, int g, int b) {
    return ((r<<1) - (b<<1)) >> 2;
}

inline int rgbToCg(int r, int g, int b) {
    return ((g<<1) - r - b)  >> 2;
}
/*
Y  =    

Cr = V =  

Cb = U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128


inline int rgbToY(int r, int g, int b) {
    return (int)(  (0.257f * R) + (0.504f * G) + (0.098f * B) + 16.0f );
}

inline int rgbToU(int r, int g, int b) {
    return (int)( (0.439f * R) - (0.368f * G) - (0.071f * B) + 128.0f );
}

inline int rgbToV(int r, int g, int b) {
    return (int)( -(0.148f * R) - (0.291f * G) + (0.439f * B) + 128.0f );
}*/