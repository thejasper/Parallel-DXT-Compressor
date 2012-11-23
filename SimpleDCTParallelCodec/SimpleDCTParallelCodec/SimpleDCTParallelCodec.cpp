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

// SimpleDCTParallelCodec.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "SimpleDCTDec.h"
#include "SimpleDCTEnc.h"
#include "SimpleDXTEnc.h"
#include "FileSystem.h"
#include "TGAImage.h"


int main(int argc, char* argv[])
{
	// Our input parameters
	std::string inputFilename = "C:\\Users\\Jasper\\Dropbox\\School\\Masterjaar\\Eerste semester\\GPU programmering\\Project\\alchemist_goblin_head_color.tga";
	
	unsigned char* pOriginal;
	int width;
	int height;
	
	std::cout << "Reading TGA..." << std::endl;
	TGAImage inputImage; 
	inputImage.Read(inputFilename, width, height, &pOriginal); // TGA contains 24-bit pixels (BGR)
	size_t pictureSize = width*height*4;	// We use 32-bit pixels (RGBA)
	// pOriginal now contains the entire input file

	
	std::cout << "Compressing TGA with JPEG YCoCg..." << std::endl;
	// We allocate memory to store the compresses file (presume double the amount of bits will suffice)
	unsigned char* pCompressed = new unsigned char[pictureSize*2];
	int compressedSize = 0;
	SimpleDCTEnc encoder;
	if (!encoder.compress(pCompressed, pOriginal, width, height, compressedSize))
	{
		fprintf(stderr, "Error compressing input file\n");
		return -1;
	}
	// pCompressed now contains our compressed file
	FileSystem::WriteMemoryToFile("Compressed.out", pCompressed, compressedSize);
	
	
	std::cout << "Decompressing JPEG YCoCg..." << std::endl;
	unsigned char* pDecompressedBGRA = new unsigned char[pictureSize];
	SimpleDCTDec decoder;
	if (!decoder.decompress(pDecompressedBGRA, pCompressed, width, height, compressedSize))
	{
		fprintf(stderr, "Error decompressing buffer\n");
		return -1;
	}
	// pCompressed now contains our compressed file	
	// Write output to disk
	TGAImage outputImage;
	outputImage.Write(width, height, pDecompressedBGRA, "Decompressed.tga");	

	std::cout << "Compressing with DXT..." << std::endl;
	SimpleDXTEnc dxtEncoder;
	unsigned char* pDXTCompressed = new unsigned char[pictureSize*2];
	if (!dxtEncoder.compress(pDXTCompressed, pDecompressedBGRA, width, height, compressedSize))
	{
		fprintf(stderr, "Error compressing buffer with dxt\n");
		return -1;
	}
	FileSystem::WriteMemoryToFile("DXTCompressed.dxt", pDXTCompressed, compressedSize);
	
	std::cout << "Exiting..." << std::endl;
	
	delete pOriginal;
	delete pCompressed;
	delete pDecompressedBGRA;
	delete pDXTCompressed;

	return 0;
	
}

