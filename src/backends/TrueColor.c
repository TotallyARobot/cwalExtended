/*
 *  cwal: Blazing-fast pywal-like color palette generator written in C.
 *  Copyright (c) 2026 Nitin Bhat <nitinbhat972@gmail.com>
 *  Repository: https://github.com/nitinbhat972/cwal
 *
 *  Licensed under the GNU General Public License v3.0.
 *  If you find this code useful, please consider giving it a star on GitHub!
 *  Any contributions or forks must retain this original header.
 */

// Backend which strives to generate more natural colors.
#include "backend.h"
#include "magickwand.h"

int findLargest (MagickWand* wand, PixelWand* pixel, int hue, int saturation, int lightness){
	double deltaHue = 100;
	double deltaLightness = 100;
	double deltaSaturation = 100;
	int index = -1;
	int range = 30;
	double lowerBound, upperBound;
 restart:
	lowerBound = hue - range;
	upperBound = hue + range;

	double *pixelHue = malloc(sizeof(double));
	double *pixelSaturation = malloc(sizeof(double));
	double *pixelLightness = malloc(sizeof(double));

	bool testcon;

	for(int i = 0; MagickGetImageColormapColor(wand, i, pixel) != MagickFalse; i++){
		PixelGetHSL(pixel, pixelHue, pixelSaturation, pixelLightness);
		//If the bounds wrap around to 0 or 360 we need to treat them acordingly, I don't focus on
		//both wraping over since that's unlikely to happen, and if it does it should still fail
		//gracefully.
		if (lowerBound < 0)
			testcon = (*pixelHue >= 0.0/360.0 && *pixelHue <= upperBound/360.0) ||
				(*pixelHue >= (360.0+lowerBound)/360.0 && *pixelHue <= 360.0/360.0);
		else if ( upperBound > 360)
			testcon = (*pixelHue >= lowerBound/360.0 && *pixelHue <= 360/360.0) ||
				(*pixelHue >= 0/360.0 && *pixelHue <= (upperBound-360.0)/360.0);
		else
			testcon = *pixelHue >= lowerBound/360.0 && *pixelHue <= upperBound/360.0;
				
		if (testcon && ((deltaHue > fabs(*pixelHue - hue/360.0) || deltaHue > fabs(*pixelHue - hue/360.0 - 1)) &&
						deltaSaturation > fabs(*pixelSaturation - saturation/100.0) &&
						deltaLightness > fabs(*pixelLightness - lightness/100.0))){
			deltaHue = fabs(*pixelHue - hue/360.0);
			deltaSaturation = fabs(*pixelSaturation - saturation/100.0);
			deltaLightness = fabs(*pixelLightness - .50);
			index = i;
			
		}
	}
	
	free(pixelHue);
	free(pixelLightness);
	free(pixelSaturation);
	if (index == -1){
		range +=5;
		goto restart;
	}
	return index;
}

static void init_magickwand() {
  if (!IsMagickWandInstantiated()) {
    MagickWandGenesis();
  }
}

static void terminate_magickwand() {
  if (IsMagickWandInstantiated()) {
    MagickWandTerminus();
  }
}

static int generate_palette_with_colors(RawImage *image, Palette *palette, int numColors) {
  if (!image || !palette || !image->pixels) {
    return -1;
  }

  MagickWand *wand = NewMagickWand();
  if (!wand) {
    return -1;
  }

  if (MagickConstituteImage(wand, image->width, image->height, "RGBA",
                            CharPixel, image->pixels) == MagickFalse) {
    DestroyMagickWand(wand);
    return -1;
  }

  // Quantize the image and output numColors amount of colors.
  if (MagickQuantizeImage(wand, numColors, sRGBColorspace, 0, NoDitherMethod,
                          MagickFalse) == MagickFalse) {
    DestroyMagickWand(wand);
    return -1;
  }

  PixelWand *pixel = NewPixelWand();
  if (!pixel) {
    DestroyMagickWand(wand);
    return -1;
  }

  // Find the index of the most, red green, yellow, blue, magenta, and cyan colors in the quantized
  // image.
  // Note: I reduced the saturation of the colors since I find it produces more visually pleasing
  // colors.
  int LargestRedIndex = findLargest(wand, pixel, 0, 25, 50);
  int LargestGreenIndex = findLargest(wand, pixel, 120, 25, 50);
  int LargestYellowIndex = findLargest(wand, pixel, 60, 25, 50);
  int LargestBlueIndex = findLargest(wand, pixel, 240, 25, 50);
  int LargestMagentaIndex = findLargest(wand, pixel, 300, 25, 50);
  int LargestCyanIndex = findLargest(wand, pixel, 180, 25, 50); 
  
  //Generate Black and White by just finding the highest and lowest lightness values in the
  //quantized image palette.
  int LargestBlackIndex = -1, LargestWhiteIndex = -1;
  double LargestLightness = 0, LowestLightness = 100;
  
  double *pixelHue = malloc(sizeof(double));
  double *pixelSaturation = malloc(sizeof(double));
  double *pixelLightness = malloc(sizeof(double));
  
  for(int i = 0; MagickGetImageColormapColor(wand, i, pixel) != MagickFalse; i++){
	  PixelGetHSL(pixel, pixelHue, pixelSaturation, pixelLightness);
	  if (LargestLightness < *pixelLightness){
		  LargestLightness = *pixelLightness;
		  LargestWhiteIndex = i;
	  }
	  if (LowestLightness > *pixelLightness){
		  LowestLightness = *pixelLightness;
		  LargestBlackIndex = i;
	  }
  }
  
  free(pixelHue);
  free(pixelLightness);
  free(pixelSaturation);
  
  int status = 0;

  //set Black
  if (MagickGetImageColormapColor(wand, LargestBlackIndex, pixel) == MagickFalse){ status = -1;}
  palette->colors[0] = (Color){
	  .red = (uint8_t)(PixelGetRed(pixel) * 255),
	  .green = (uint8_t)(PixelGetGreen(pixel) * 255),
	  .blue = (uint8_t)(PixelGetBlue(pixel) * 255),
  };
  //set Red
  if (MagickGetImageColormapColor(wand, LargestRedIndex, pixel) == MagickFalse){ status = -1;}
  palette->colors[1] = (Color){
	  .red = (uint8_t)(PixelGetRed(pixel) * 255),
	  .green = (uint8_t)(PixelGetGreen(pixel) * 255),
	  .blue = (uint8_t)(PixelGetBlue(pixel) * 255),
  };
  //set Green
  if (MagickGetImageColormapColor(wand, LargestGreenIndex, pixel) == MagickFalse){ status = -1;}
  palette->colors[2] = (Color){
	  .red = (uint8_t)(PixelGetRed(pixel) * 255),
	  .green = (uint8_t)(PixelGetGreen(pixel) * 255),
	  .blue = (uint8_t)(PixelGetBlue(pixel) * 255),
  };
  //set Yellow
  if (MagickGetImageColormapColor(wand, LargestYellowIndex, pixel) == MagickFalse){ status = -1;}
  palette->colors[3] = (Color){
	  .red = (uint8_t)(PixelGetRed(pixel) * 255),
	  .green = (uint8_t)(PixelGetGreen(pixel) * 255),
	  .blue = (uint8_t)(PixelGetBlue(pixel) * 255),
  };
  //set Blue
  if (MagickGetImageColormapColor(wand, LargestBlueIndex, pixel) == MagickFalse){ status = -1;}
  palette->colors[4] = (Color){
	  .red = (uint8_t)(PixelGetRed(pixel) * 255),
	  .green = (uint8_t)(PixelGetGreen(pixel) * 255),
	  .blue = (uint8_t)(PixelGetBlue(pixel) * 255),
  };
  //set Magenta
  if (MagickGetImageColormapColor(wand, LargestMagentaIndex, pixel) == MagickFalse){ status = -1;}
  palette->colors[5] = (Color){
	  .red = (uint8_t)(PixelGetRed(pixel) * 255),
	  .green = (uint8_t)(PixelGetGreen(pixel) * 255),
	  .blue = (uint8_t)(PixelGetBlue(pixel) * 255),
  };
  //set Cyan
  if (MagickGetImageColormapColor(wand, LargestCyanIndex, pixel) == MagickFalse){ status = -1;}
  palette->colors[6] = (Color){
	  .red = (uint8_t)(PixelGetRed(pixel) * 255),
	  .green = (uint8_t)(PixelGetGreen(pixel) * 255),
	  .blue = (uint8_t)(PixelGetBlue(pixel) * 255),
  };
  //set White
  if (MagickGetImageColormapColor(wand, LargestWhiteIndex, pixel) == MagickFalse){ status = -1;}
  palette->colors[7] = (Color){
	  .red = (uint8_t)(PixelGetRed(pixel) * 255),
	  .green = (uint8_t)(PixelGetGreen(pixel) * 255),
	  .blue = (uint8_t)(PixelGetBlue(pixel) * 255),
  };  
	  
  DestroyPixelWand(pixel);
  DestroyMagickWand(wand);
  return status;
}

static int generate_palette_512(RawImage *image, Palette *palette){
	return generate_palette_with_colors(image, palette, 512);
}
static int generate_palette_256(RawImage *image, Palette *palette){
	return generate_palette_with_colors(image, palette, 256);
}
static int generate_palette_128(RawImage *image, Palette *palette){
	return generate_palette_with_colors(image, palette, 128);
}
static int generate_palette_64(RawImage *image, Palette *palette){
	return generate_palette_with_colors(image, palette, 64);
}
static int generate_palette_32(RawImage *image, Palette *palette){
	return generate_palette_with_colors(image, palette, 32);
}
static int generate_palette_16(RawImage *image, Palette *palette){
	return generate_palette_with_colors(image, palette, 16);
}
static int generate_palette_8(RawImage *image, Palette *palette){
	return generate_palette_with_colors(image, palette, 8);
}

ImageBackend truecolor512 = {.name = "truecolor512",
                     .init_backend = init_magickwand,
                     .terminate_backend = terminate_magickwand,
                     .generate_palette = generate_palette_512};
ImageBackend truecolor256 = {.name = "truecolor256",
                     .init_backend = init_magickwand,
                     .terminate_backend = terminate_magickwand,
                     .generate_palette = generate_palette_256};
ImageBackend truecolor128 = {.name = "truecolor128",
                     .init_backend = init_magickwand,
                     .terminate_backend = terminate_magickwand,
                     .generate_palette = generate_palette_128};
ImageBackend truecolor64 = {.name = "truecolor64",
                     .init_backend = init_magickwand,
                     .terminate_backend = terminate_magickwand,
                     .generate_palette = generate_palette_64};
ImageBackend truecolor32 = {.name = "truecolor32",
                     .init_backend = init_magickwand,
                     .terminate_backend = terminate_magickwand,
                     .generate_palette = generate_palette_32};
ImageBackend truecolor16 = {.name = "truecolor16",
                     .init_backend = init_magickwand,
                     .terminate_backend = terminate_magickwand,
                     .generate_palette = generate_palette_16};
ImageBackend truecolor8 = {.name = "truecolor8",
                     .init_backend = init_magickwand,
                     .terminate_backend = terminate_magickwand,
                     .generate_palette = generate_palette_8};
