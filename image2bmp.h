#ifndef IMAGE2BMP_H
#define IMAGE2BMP_H

enum image_format {
	UNKNOW_FORMAT,
	JPEG_FORMAT,
	PNG_FORMAT,
	SPI_FORMAT,
};

int image2bmp(const char *input_file, const char *output_file);
#endif


