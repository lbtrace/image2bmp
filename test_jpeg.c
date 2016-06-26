/* test code convert JPEG to RGBA_8888 */
#include <stdio.h>
#include <string.h>

#include "image2bmp.h"

#define MAX_FILE_NAME 128

static char input[MAX_FILE_NAME];
static char output[MAX_FILE_NAME];
int main(int argc, char **argv)
{
	int input_len;
	int output_len;

	if (argc < 3) {
		printf("please add JPEG file and output file!\n");
		return 1;
	}

	input_len = strlen(argv[1]);
	output_len = strlen(argv[2]);

	if (input_len > (MAX_FILE_NAME - 1) ||
			output_len > (MAX_FILE_NAME - 1)) {
		printf("input or output file name is too long!\n");
		return 1;
	}

	strncpy(input, argv[1], input_len);
	strncpy(output, argv[2], output_len);

	if (image2bmp(input, output))
		printf("convert JPEG to RGBA success!\n");
	else
		printf("convert JPEG to RGBA fail!\n");

	return 1;
}
