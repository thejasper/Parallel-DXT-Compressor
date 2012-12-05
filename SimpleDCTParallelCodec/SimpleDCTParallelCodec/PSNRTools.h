#pragma once

namespace PSNRTools
{	
	struct PSNR_PLANE_INFO
	{
		double psnr;
		unsigned long mse_min;
		unsigned long mse_max;
		unsigned long long mse_avg;
	};

	struct PSNR_INFO
	{
		PSNR_PLANE_INFO y;
		PSNR_PLANE_INFO u;
		PSNR_PLANE_INFO v;
	};

	void ConvertFromBGRAtoRGBA(unsigned char* input, int pixel_width, int pixel_height);
	void CalculatePSNRFromRGBA(PSNR_INFO& psnr, const unsigned char* rgba1, const unsigned char* rgba2, int width, int height);
}