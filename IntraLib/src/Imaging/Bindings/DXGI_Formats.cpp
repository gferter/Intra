﻿#include "Imaging/Bindings/DXGI_Formats.h"
#include "Imaging/ImagingTypes.h"

namespace Intra {

static const ImageFormat dxgiFormatConvertTable[]=
{
	null,
	null, ImageFormat::RGBA32f, ImageFormat::RGBA32u, ImageFormat::RGBA32i,
	null, ImageFormat::RGB32f,  ImageFormat::RGB32u,  ImageFormat::RGB32i,
	null, ImageFormat::RGBA16f, ImageFormat::RGBA16, ImageFormat::RGBA16u, ImageFormat::RGBA16s, ImageFormat::RGBA16i,
	null, ImageFormat::RG32f, ImageFormat::RG32u, ImageFormat::RG32i,
	null, ImageFormat::Depth32fStencil8, null, null,
	null, ImageFormat::RGB10A2, ImageFormat::RGB10A2u,
	ImageFormat::R11G11B10f,
	null, ImageFormat::RGBA8, ImageFormat::sRGB8_A8, ImageFormat::RGBA8u, ImageFormat::RGBA8s, ImageFormat::RGBA8i,
	null, ImageFormat::RG16f, ImageFormat::RG16, ImageFormat::RG16u, ImageFormat::RG16s, ImageFormat::RG16i,
	null, ImageFormat::Depth32f, ImageFormat::Red32f, ImageFormat::Red32u, ImageFormat::Red32i,
	null, ImageFormat::Depth24Stencil8, null, null,
	null, ImageFormat::RG8, ImageFormat::RG8u, ImageFormat::RG8s, ImageFormat::RG8i,
	null, ImageFormat::Red16f, ImageFormat::Depth16, ImageFormat::Red16, ImageFormat::Red16u, ImageFormat::Red16s, ImageFormat::Red16i,
	null, ImageFormat::Red8, ImageFormat::Red8u, ImageFormat::Red8s, ImageFormat::Red8i, ImageFormat::Alpha8,
	null, ImageFormat::RGB9E5, null, null,

	null, ImageFormat::DXT1_RGB, ImageFormat::DXT1_sRGB,
	null, ImageFormat::DXT3_RGBA, ImageFormat::DXT3_sRGB_A,
	null, ImageFormat::DXT5_RGBA, ImageFormat::DXT5_sRGB_A,
	null, ImageFormat::RGTC_Red, ImageFormat::RGTC_SignedRed,
	null, ImageFormat::RGTC_RG, ImageFormat::RGTC_SignedRG,

	ImageFormat::RGB565, ImageFormat::RGB5A1,
	ImageFormat::RGBA8,
	ImageFormat::RGBA8, //Альфа не должна использоваться!
	null,
	null, ImageFormat::sRGB8_A8,
	null, ImageFormat::sRGB8_A8, //Альфа не должна использоваться!

	null, ImageFormat::BPTC_RGBuf, ImageFormat::BPTC_RGBf,
	null, ImageFormat::BPTC_RGBA, ImageFormat::BPTC_sRGB_A,

	null, null, null, null, null, null, null, null, null, null, null, null, null, null, null,
	ImageFormat::RGBA4
};
INTRA_CHECK_TABLE_SIZE(dxgiFormatConvertTable, DXGI_FORMAT_B4G4R4A4_UNORM+1);

ImageFormat DXGI_ToImageFormat(DXGI_FORMAT fmt, bool* swapRB)
{
	if(swapRB!=null) *swapRB = (
		fmt>=DXGI_FORMAT_B5G6R5_UNORM && fmt<=DXGI_FORMAT_B8G8R8X8_UNORM_SRGB ||
		fmt==DXGI_FORMAT_B4G4R4A4_UNORM);
	if(fmt>=core::numof(dxgiFormatConvertTable)) return null;
	return dxgiFormatConvertTable[fmt];
}

DXGI_FORMAT DXGI_FromImageFormat(ImageFormat fmt, bool swapRB)
{
	static DXGI_FORMAT map_no_swap[ImageFormat::End], map_swap[ImageFormat::End];
	static bool inited = false;
	if(!inited)
	{
		core::memset(map_no_swap, DXGI_FORMAT_UNKNOWN, sizeof(map_no_swap));
		core::memset(map_swap, DXGI_FORMAT_UNKNOWN, sizeof(map_swap));
		for(int i=DXGI_FORMAT_UNKNOWN+1; i<=DXGI_FORMAT_B4G4R4A4_UNORM; i++)
		{
			bool swap;
			auto dxgi = DXGI_FORMAT(i);
			auto f = DXGI_ToImageFormat(dxgi, &swap).value;
			if(!swap) map_no_swap[f] = dxgi;
			if(swap) map_swap[f] = dxgi;
		}
		inited = true;
	}
	return (swapRB? map_swap: map_no_swap)[fmt.value];
}

}
