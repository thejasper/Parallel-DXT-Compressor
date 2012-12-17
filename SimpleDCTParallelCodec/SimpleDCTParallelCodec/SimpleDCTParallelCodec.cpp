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

#include <sstream>
#include <vector>
#include <Windows.h>

#include "SimpleDCTDec.h"
#include "SimpleDCTEnc.h"
#include "SimpleDXTEnc.h"
#include "FileSystem.h"
#include "TGAImage.h"
#include "PSNRTools.h"
#include "squish.h"

using namespace std;

int main(int argc, char* argv[])
{
	unsigned char* pOriginal;
	int width, height;

	// our default input parameters
	string inputFilename = "alchemist_goblin_head_color.tga";
	string outputFilename = "DXTCompressed.dds";
	bool verbose = false;
	int loops = 1;
	
	// our timer parameters
	LARGE_INTEGER startTime, endTime, freq;

	// parse command line arguments
	for (int i = 1; i < argc; ++i) 
	{
		if (strcmp(argv[i],  "-i") == 0) 
			inputFilename = argv[++i];
		else if (strcmp(argv[i], "-o") == 0) 
			outputFilename = argv[++i];
		else if (strcmp(argv[i], "-verbose") == 0) 
			verbose = true;
		else if (strcmp(argv[i], "-l") == 0) 
		{
			stringstream ss(argv[++i]);
			ss >> loops;

			if (loops <= 0)
				fprintf(stderr, "Error reading loop count, set to 1\n");
		}
	}

	// read tga image
	if (verbose) cout << "Reading TGA..." << endl;
	TGAImage inputImage; 
	inputImage.Read(inputFilename, width, height, &pOriginal);
	size_t pictureSize = width*height*4;

	// compress to jpeg
	if (verbose) cout << "Compressing TGA with JPEG YCoCg..." << endl;
	unsigned char* pCompressed = new unsigned char[pictureSize*2];
	int compressedSize = 0;
	SimpleDCTEnc encoder;
	if (!encoder.compress(pCompressed, pOriginal, width, height, compressedSize))
	{
		fprintf(stderr, "Error compressing input file\n");
		return -1;
	}
	FileSystem::WriteMemoryToFile("Compressed.out", pCompressed, compressedSize);
	
	// decompress jpeg
	if (verbose) cout << "Decompressing JPEG YCoCg..." << endl;
	unsigned char* pDecompressedBGRA = new unsigned char[pictureSize];
	SimpleDCTDec decoder;
	if (!decoder.decompress(pDecompressedBGRA, pCompressed, width, height, compressedSize))
	{
		fprintf(stderr, "Error decompressing buffer\n");
		return -1;
	}
	TGAImage outputImage;
	outputImage.Write(width, height, pDecompressedBGRA, "Decompressed.tga");	

	// compress to dxt1
	if (verbose) cout << "Compressing with DXT..." << endl;
	SimpleDXTEnc dxtEncoder(pDecompressedBGRA, width, height);
	unsigned char* pDXTCompressed = new unsigned char[pictureSize*2];
	QueryPerformanceCounter(&startTime);
	for (int i = 0; i < loops; i++) 
	{
		if (verbose) cout << '.';
		if (!dxtEncoder.compress(pDXTCompressed, compressedSize))
		{
			fprintf(stderr, "Error compressing buffer with dxt\n");
			return -1;
		}
	}
	if (verbose) cout << endl;
	QueryPerformanceCounter(&endTime);
	QueryPerformanceFrequency(&freq);

	// write results
	FileSystem::WriteMemoryToFile(outputFilename, pDXTCompressed, compressedSize);

	// dxt decompressen (squish gebruikt rgba volgorde)
	unsigned char* pDecompressedDXT = new unsigned char[width * height * 4];
	squish::DecompressImage(pDecompressedDXT, width, height, pDXTCompressed + 128, squish::kDxt1);
	
	unsigned char* pDecompressedRGBA = pDecompressedBGRA; // Nieuwe pointer om naamverwarring te voorkomen
	PSNRTools::ConvertFromBGRAtoRGBA(pDecompressedRGBA, width, height); // Kleurvolgorde aanpassen, pDecompressedBGRA is niet meer geldig als naam

	// PSNR bepalen en resultaat uitvoeren
	PSNRTools::PSNR_INFO psnrResult;
	PSNRTools::CalculatePSNRFromRGBA(psnrResult, pDecompressedDXT, pDecompressedRGBA, width, height);
	float averageTime = (float)(endTime.LowPart - startTime.LowPart) * 1000 / (freq.LowPart * loops);
	
	cout << averageTime << ", " << psnrResult.y.psnr << ", " << "Desmadryl" << ", Cockaerts" << endl;

	if (verbose) cout << "Exiting..." << endl;
	
	// clean up
	delete pOriginal;
	delete pCompressed;
	delete pDecompressedBGRA;
	delete pDXTCompressed;
	delete pDecompressedDXT;

	return 0;
}
