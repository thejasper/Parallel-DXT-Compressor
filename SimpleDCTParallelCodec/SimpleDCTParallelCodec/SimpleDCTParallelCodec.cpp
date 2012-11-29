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

#include <vector>

#include "amp.h"

#include "SimpleDCTDec.h"
#include "SimpleDCTEnc.h"
#include "SimpleDXTEnc.h"
#include "FileSystem.h"
#include "TGAImage.h"
#include <Windows.h>
#include "PSNRTools.h"
#include <sstream>

using namespace concurrency;

int main(int argc, char* argv[])
{
	// Our input parameters
	std::string inputFilename = "input.tga";
	std::string outputFilename = "DXTCompressed.dds";
	bool verbose = false;
	int loops = 1;
	
	// Our timer parameters
	LARGE_INTEGER startTime, endTime, freq;

	std::vector<accelerator> accs = accelerator::get_all();
    for (int i = 0; i < accs.size(); i++) {
        std::wcout << accs[i].device_path << "\n";
        std::wcout << accs[i].dedicated_memory << "\n";
        std::wcout << (accs[i].supports_double_precision ? 
            "double precision: true" : "double precision: false") << "\n";    
    }

	for (int i=1; i<argc; ++i) {
		if (strcmp(argv[i],  "-i") == 0) {
			i++;
			inputFilename = argv[i];
		} else if (strcmp(argv[i], "-o") == 0) {
			i++;
			outputFilename = argv[i];
		} else if (strcmp(argv[i], "-verbose") == 0) {
			verbose = true;
		} else if (strcmp(argv[i], "-l") == 0) {
			i++;
			std::string sgNumber = argv[i];
			std::stringstream smNumber(sgNumber);

			smNumber >> loops;
			if(loops == 0) {
				fprintf(stderr, "Error reading loop count, set to 1\n");
				loops = 1;
			}
		}
	}

	unsigned char* pOriginal;
	int width;
	int height;
	
	if(verbose) std::cout << "Reading TGA..." << std::endl;
	TGAImage inputImage; 
	inputImage.Read(inputFilename, width, height, &pOriginal); // TGA contains 24-bit pixels (BGR)
	size_t pictureSize = width*height*4;	// We use 32-bit pixels (RGBA)
	// pOriginal now contains the entire input file

	
	if(verbose) std::cout << "Compressing TGA with JPEG YCoCg..." << std::endl;
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
	
	
	if(verbose) std::cout << "Decompressing JPEG YCoCg..." << std::endl;
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

	if(verbose) std::cout << "Compressing with DXT..." << std::endl;
	SimpleDXTEnc dxtEncoder(pDecompressedBGRA, width, height);
	unsigned char* pDXTCompressed = new unsigned char[pictureSize*2];
	QueryPerformanceCounter(&startTime);  //Start of the timer
	for (int i=1; i<=loops; i++) {
		if(verbose) std::cout << "-" << i;
		if (!dxtEncoder.compress(pDXTCompressed, compressedSize))
		{
			fprintf(stderr, "Error compressing buffer with dxt\n");
			return -1;
		}
	}
	if(verbose)  std::cout << std::endl;
	QueryPerformanceCounter(&endTime); //End of the timer
	QueryPerformanceFrequency(&freq); //Get the frequency
	//TODO Synchronise with GPU!

	//Calculate PSNR Value
	PSNRTools::PSNR_INFO psnrResult;
	PSNRTools::CalculatePSNRFromRGBA(psnrResult, pDXTCompressed, pDecompressedBGRA, width, height);
	double averagePSNR = (psnrResult.u.psnr + psnrResult.v.psnr + psnrResult.y.psnr) / 3;

	FileSystem::WriteMemoryToFile(outputFilename, pDXTCompressed, compressedSize);

	float averageTime = (float)(endTime.LowPart - startTime.LowPart)*1000/(freq.LowPart * loops); //Calculate the average time
	
	std::cout << averageTime << ", " << averagePSNR << ", ";
	std::cout << "Desmadryl" << ", Cockaerts" << std::endl;

	if(verbose)system("pause");
	if(verbose) std::cout << "Exiting..." << std::endl;
	
	delete pOriginal;
	delete pCompressed;
	delete pDecompressedBGRA;
	delete pDXTCompressed;

	return 0;
}

