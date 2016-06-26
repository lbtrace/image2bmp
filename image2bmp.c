/*
 * Convert common image format encode to RGBA_8888(BMP) for Android Device
 *
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <fcntl.h>
#include <linux/fb.h>

#include "image2bmp.h"
// Support JPEG format
#include "jpeglib.h"
#include "jerror.h"

#define DBG_JPEG 1
#define FB_PATH "/dev/graphics/fb0"

// record device screen info
struct FB {
	int fd;
	struct fb_var_screeninfo vi;
};

// a pixel of RGBA_8888 
typedef struct rgba {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
}rgba_t;

// bmp after decode JPEG  
static rgba_t  *src_bmp;
// final bmp after scale
static rgba_t  *dest_bmp;

static unsigned int src_h;
static unsigned int src_w;
static unsigned int dest_h = 1280;
static unsigned int dest_w = 720;

// Get screen info from FB driver
static void init_dest_size(void)
{
	struct FB *fb;

	fb = (struct FB *)malloc(sizeof(struct FB));
	fb->fd = open(FB_PATH, O_RDWR);
	if (fb->fd < 0) {
		// TODO
		return;
	}

	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vi) < 0)
		return;

	dest_w = fb->vi.xres;
	dest_h = fb->vi.yres;
	fprintf(stderr, "device screen : %d X %d\n", dest_h, dest_w);
	
	close(fb->fd);
	free(fb);
}

/* 
 * Nearest algorithm scale bmp
 *
 * @trans_w: bmp translation at screen width direction
 * @trans_h: bmp translation at screen height direction
 * @scale: bmp scale factor
 */
static void nearest_algorithm(int trans_w, int trans_h, float scale)
{
	int i, j;

	fprintf(stderr, "trans_w %d, trans_h  %d, scale %f \n", trans_w, trans_h, scale);
	// src_x = dest_x * scale; src_y = dest_y * scale
	for (i = trans_h; i < dest_h; i++) {
		for (j = trans_w; j < dest_w; j++) {
			int si = (int)((i - trans_h) * scale);
			int sj = (int)((j - trans_w) * scale);

			if (si < src_h && sj < src_w)
				dest_bmp[i * dest_w + j] = src_bmp[si * src_w + sj];
		}
	}
}

static void scale_bmp(void)
{
	int trans_w = 0;
	int trans_h = 0;
	float scale = 1.0;

	// equal ratio scale
	if (src_w > dest_w || src_h > dest_h) {
		float scale_w = (float)src_w / dest_w;
		float scale_h = (float)src_h / dest_h;

		if (scale_w > scale_h) {
			scale = scale_w;
			trans_h = (dest_h - (int)(src_h * dest_w / src_w)) / 2;
		} else {
			scale = scale_h;
			trans_w = (dest_w - (int)(src_w * dest_h / src_h)) / 2;
		}
	} else {
		trans_w = (dest_w - src_w) / 2;
		trans_h = (dest_h - src_h) / 2;
	}

	nearest_algorithm(trans_w, trans_h, scale);
}

/* Support JPEG format */
struct jpeg2bmp_err_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};
typedef struct jpeg2bmp_err_mgr *jpeg2bmp_err_ptr;

static void jpeg2bmp_err_exit(j_common_ptr cinfo)
{
	jpeg2bmp_err_ptr j2b_err = (jpeg2bmp_err_ptr)cinfo->err;

	(*cinfo->err->output_message)(cinfo);
	longjmp(j2b_err->setjmp_buffer, 1);
}

static inline void set_bmp_format(struct jpeg_decompress_struct *cinfo, J_COLOR_SPACE color)
{
	if (cinfo)
		cinfo->out_color_space = color;
}

static void alloc_bmp_buf(void)
{
	src_bmp = (rgba_t *)malloc(src_h * src_w * sizeof(rgba_t));
	if (!src_bmp) {
		fprintf(stderr, "src_bmp alloc fail!");
		return;
	}

	dest_bmp = (rgba_t *)calloc(dest_h * dest_w, sizeof(rgba_t)); 
	if (!dest_bmp) {
		fprintf(stderr, "dest_bmp alloc fail!");
		return;
	}
}

static void free_bmp_buf(void)
{
	if (src_bmp)
		free(src_bmp);

	if (dest_bmp)
		free(dest_bmp);
}

/*
 * Convert JPEG to RGBA_8888 format 
 * For detail please check /external/jpeg/example.c 
 *
 * @filename: input JPEG file name
 * @return: 1 success or 0 fail
 */
static int jpeg2bmp(const char *input_file)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg2bmp_err_mgr j2b_err;
	FILE *jpegfile;
	rgba_t *tmp_src;
	int ret = 0;

	if ((jpegfile = fopen(input_file, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", input_file);
		goto open_jpegfile_err;
	}

	jpeg_create_decompress(&cinfo);
	cinfo.err = jpeg_std_error(&j2b_err.pub);
	j2b_err.pub.error_exit = jpeg2bmp_err_exit;
	if (setjmp(j2b_err.setjmp_buffer)) {
		goto setjmp_err;
	}

	jpeg_stdio_src(&cinfo, jpegfile);
	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
		fprintf(stderr, "jpeg read header err\n");
		goto jpeg_read_header_err;
	}

	// Android add RGBA_8888 format in libjpeg
	set_bmp_format(&cinfo, JCS_RGBA_8888);
	jpeg_start_decompress(&cinfo);

	src_h = cinfo.image_height;
	src_w = cinfo.image_width;
	if (DBG_JPEG)
		fprintf(stderr, "input width %u height %u <%s : %d line>\n",
				src_w, src_h, __FILE__, __LINE__);
		
	alloc_bmp_buf();
	tmp_src = src_bmp;
	while (cinfo.output_scanline < cinfo.output_height) {
		if (jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&tmp_src, 1) == 0)
			break;

		tmp_src += src_w;
	}

	ret = 1;
	jpeg_finish_decompress(&cinfo);
jpeg_read_header_err:
setjmp_err:
	jpeg_destroy_decompress(&cinfo);
	fclose(jpegfile);
open_jpegfile_err:
	return ret;
}

static enum image_format get_input_format(const char *input_file)
{
	// Now only support JPEG
	return JPEG_FORMAT;
}

static void write_output_bmp(const char *output_file)
{
	FILE *bmpfile;

	if ((bmpfile = fopen(output_file, "wb")) == NULL) {
		fprintf(stderr, "can't open %s\n", output_file);
		return;
	}

	while (dest_h--) {
		fwrite(dest_bmp, dest_w * sizeof(rgba_t), 1, bmpfile);
		dest_bmp += dest_w;
	}

	fflush(bmpfile);
	fclose(bmpfile);
}

/*
 * Convert input image encode to RGBA_8888
 *
 * @input_file: input image file
 * @output_file: output RGBA raw file
 * @return: 1 success or 0 fail
 */
int image2bmp(const char *input_file, const char *output_file)
{
	int ret;
	enum image_format format = UNKNOW_FORMAT;

	init_dest_size();
	format = get_input_format(input_file);
	switch (format) {
		case UNKNOW_FORMAT:
			ret = 0;
			break;
		case JPEG_FORMAT:
			ret = jpeg2bmp(input_file);
			break;
		case PNG_FORMAT:
			ret = 0;
			break;
		default:
			ret = 0;
			break;
	}

	scale_bmp();
	write_output_bmp(output_file);
	free_bmp_buf();
	return ret;
}
