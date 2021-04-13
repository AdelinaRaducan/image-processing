/**
 * Diff tool for BMP files. What it does different than hexdumping and
 * comparing the results is that it compares the pixels in the bitmap
 * with a tolerance given as argument.
 * It has different return codes for each error, so it can be checked
 * in the testing script.
 *
 * Written by Calin Cruceru, calin_cristian.cruceru@cti.pub.ro
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#pragma pack(1)
struct bmp_fileheader
{
    unsigned char  fileMarker1;
    unsigned char  fileMarker2;
    unsigned int   bfSize;
    unsigned short unused1;
    unsigned short unused2;
    unsigned int   imageDataOffset;
};

struct bmp_infoheader
{
    unsigned int   biSize;
    signed int     width;
    signed int     height;
    unsigned short planes;
    unsigned short bitPix;
    unsigned int   biCompression;
    unsigned int   biSizeImage;
    int            biXPelsPerMeter;
    int            biYPelsPerMeter;
    unsigned int   biClrUsed;
    unsigned int   biClrImportant;
};

struct rgb_pixel
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
};
#pragma pack()

#define DIE(assertion, call_description) \
    do { \
        if (assertion) { \
            fprintf(stderr, "(%s, %d): ", \
                    __FILE__, __LINE__); \
            perror(call_description); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#ifdef DEBUG
#define dprintf printf
#else
#define dprintf(...)
#endif


int main(int argc, char *argv[])
{
    DIE(argc != 4, "Usage: ./diff_bmp image1.bmp image2.bmp $tolerance");
    dprintf("\n\n%s\n%s\n%s\n\n", argv[1], argv[2], argv[3]);

    FILE *lhs = fopen(argv[1], "rb");
    DIE(lhs == NULL, "Error while opening first image");
    FILE *rhs = fopen(argv[2], "rb");
    DIE(rhs == NULL, "Error while opening second image");

    enum {
        FILEHEADER_READ_ERROR = 1,
        INFOHEADER_READ_ERROR,
        FILEMARKER1_ERROR,
        FILEMARKER2_ERROR,
        FILESIZE_ERROR,
        IMAGEDATAOFFSET_ERROR,
        INFOHEADERSIZE_ERROR,
        WIDTH_ERROR,
        HEIGHT_ERROR,
        BITMAPSIZE_ERROR,
        XPIXELS_ERROR,
        YPIXELS_ERROR,
        PIXEL_ERROR,
        PADDINGNOTZERO_ERROR,
        BITMAPS_ERROR
    } error = 0;

    struct bmp_fileheader lhs_fhdr, rhs_fhdr;
    if (fread(&lhs_fhdr, sizeof(struct bmp_fileheader), 1, lhs) != 1 ||
        fread(&rhs_fhdr, sizeof(struct bmp_fileheader), 1, rhs) != 1) {
        error = FILEHEADER_READ_ERROR;
        goto exit_exec;
    }
    if (lhs_fhdr.fileMarker1 != rhs_fhdr.fileMarker1) {
        error = FILEMARKER1_ERROR;
        goto exit_exec;
    }
    if (lhs_fhdr.fileMarker2 != rhs_fhdr.fileMarker2) {
        error = FILEMARKER2_ERROR;
        goto exit_exec;
    }
    if (lhs_fhdr.bfSize != rhs_fhdr.bfSize) {
        error = FILESIZE_ERROR;
        goto exit_exec;
    }
    if (lhs_fhdr.imageDataOffset != rhs_fhdr.imageDataOffset) {
        error = IMAGEDATAOFFSET_ERROR;
        goto exit_exec;
    }

    struct bmp_infoheader lhs_ihdr, rhs_ihdr;
    if (fread(&lhs_ihdr, sizeof(struct bmp_infoheader), 1, lhs) != 1 ||
        fread(&rhs_ihdr, sizeof(struct bmp_infoheader), 1, rhs) != 1) {
        error = INFOHEADER_READ_ERROR;
        goto exit_exec;
    }
    if (lhs_ihdr.biSize != rhs_ihdr.biSize) {
        error = INFOHEADERSIZE_ERROR;
        goto exit_exec;
    }
    if (lhs_ihdr.width != rhs_ihdr.width) {
        error = WIDTH_ERROR;
        goto exit_exec;
    }
    if (lhs_ihdr.height != rhs_ihdr.height) {
        error = HEIGHT_ERROR;
        goto exit_exec;
    }
    if (lhs_ihdr.biSizeImage != rhs_ihdr.biSizeImage) {
        error = BITMAPSIZE_ERROR;
        goto exit_exec;
    }
    if (lhs_ihdr.biXPelsPerMeter != rhs_ihdr.biXPelsPerMeter) {
        error = XPIXELS_ERROR;
        goto exit_exec;
    }
    if (lhs_ihdr.biYPelsPerMeter != rhs_ihdr.biYPelsPerMeter) {
        error = YPIXELS_ERROR;
        goto exit_exec;
    }

    int tol = atoi(argv[3]);
    const int padding = 4 - (3*lhs_ihdr.width) % 4;

    struct rgb_pixel lhs_pix, rhs_pix;
    while (fread(&lhs_pix, 1, 3, lhs) == 3 &&
           fread(&rhs_pix, 1, 3, rhs) == 3) {
        if (abs(lhs_pix.R - rhs_pix.R) > tol ||
            abs(lhs_pix.G - rhs_pix.G) > tol ||
            abs(lhs_pix.B - lhs_pix.B) > tol) {
            error = PIXEL_ERROR;
            goto exit_exec;
        }
        if (padding > 0 && padding < 4)
            for (int i = 0; i < padding; ++i) {
                unsigned char lhs_z, rhs_z;
                if (fread(&lhs_z, 1, 1, lhs) != 1 ||
                    fread(&rhs_z, 1, 1, rhs) != 1 ||
                    lhs_z != rhs_z || lhs_z != 0) {
                    error = PADDINGNOTZERO_ERROR;
                    goto exit_exec;
                }
            }
    }
    int dummy;
    if (fread(&dummy, 1, 1, lhs) || fread(&dummy, 1, 1, rhs))
        error = BITMAPS_ERROR;

exit_exec:
    fclose(lhs);
    fclose(rhs);
    exit(error);
}
