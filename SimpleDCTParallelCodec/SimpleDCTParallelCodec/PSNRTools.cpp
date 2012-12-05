#include "stdafx.h"

#include <malloc.h>
#include <math.h>
#include <Limits.h>
#include "PSNRTools.h"


static void RGBAToYCbCr444_CPP(const unsigned char* rgba, unsigned char* y, unsigned char* u, unsigned char* v, int width, int height)
{
	for (int l=0;l<height;++l)
	{
		for (int i=0;i<width;i++, y++, u++, v++, rgba+=4)
		{
			int R, G, B, A, Y, Cr, Cb;

			R = rgba[0];
			G = rgba[1];
			B = rgba[2];
			A = rgba[3];

			Y  =  (int)( (0.257f * R) + (0.504f * G) + (0.098f * B) + 16);
			Cr =  (int)( (0.439f * R) - (0.368f * G) - (0.071f * B) + 128);
			Cb =  (int)(-(0.148f * R) - (0.291f * G) + (0.439f * B) + 128);

			*y = Y;
			*u = Cb;
			*v = Cr;			
		}	
	}
}

namespace PSNRTools
{
	void CalculatePSNRFromRGBA(PSNR_INFO& psnr, const unsigned char* rgba1, const unsigned char* rgba2, int width, int height)
	{
		int size = width*height;
		unsigned char* yuv1 = (unsigned char*)malloc(size*3);
		unsigned char* yuv2 = (unsigned char*)malloc(size*3);

		unsigned char* p1[3];
		unsigned char* p2[3];
		p1[0] = yuv1;
		p1[1] = yuv1 + 1*width*height;
		p1[2] = yuv1 + 2*width*height;
		p2[0] = yuv2;
		p2[1] = yuv2 + 1*width*height;
		p2[2] = yuv2 + 2*width*height;

		int psize[3] ={ width*height, width*height / 4, width*height / 4 };


		RGBAToYCbCr444_CPP(rgba1, p1[0], p1[1], p1[2], width, height);
		RGBAToYCbCr444_CPP(rgba2, p2[0], p2[1], p2[2], width, height);

		PSNR_PLANE_INFO pinfo[3];
		for (int plane=0;plane<3;++plane)
		{
			pinfo[plane].mse_avg = 0;
			pinfo[plane].mse_max = 0;
			pinfo[plane].mse_min = LONG_MAX;		
			unsigned long long ssd = 0;

			for (int i=0;i<width*height;++i)
			{
				long diff = p1[plane][i] - p2[plane][i];

				if ((unsigned long)(diff*diff) < pinfo[plane].mse_min)
					pinfo[plane].mse_min = diff*diff;
				if ((unsigned long)(diff*diff) > pinfo[plane].mse_max)
					pinfo[plane].mse_max = diff*diff;
				pinfo[plane].mse_avg += diff*diff;

				ssd += diff*diff;
			}

			pinfo[plane].mse_avg = pinfo[plane].mse_avg / psize[plane];

			if( ssd == 0 ) 
				pinfo[plane].psnr = 1e100;
			else 
				pinfo[plane].psnr = 10.0 * log10( 255.0*255.0 / ((double)ssd / (double)size) );		
		}

		psnr.y = pinfo[0];
		psnr.u = pinfo[1];
		psnr.v = pinfo[2];


		free(yuv1);
		free(yuv2);
	}

	void ConvertFromBGRAtoRGBA(unsigned char* input, int pixel_width, int pixel_height)
	{
		int offset = 0;
		unsigned char temp;

		for (int y = 0; y < pixel_height; y++) {
			for (int x = 0; x < pixel_width; x++) {
				temp = input[offset];
				input[offset] = input[offset + 2];
				input[offset + 2] = temp;

				offset += 4;
			}
		}
	}
}