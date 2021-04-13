
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#define UNUSED(x) (void)(x)

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

typedef struct 
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
} BGR_pixel;
#pragma pack()

void        read_pixel                  (BGR_pixel *pixel, FILE *inputFile);
void        read_input_pixels           (BGR_pixel *ref, BGR_pixel *offset, double *clusterSizeFactor, char *inputFilename);
void        read_header                 (struct bmp_fileheader *imageFileheader, struct bmp_infoheader *imageInfoheader, fpos_t *position, FILE *inputFile);
BGR_pixel **alloc_image_pixels          (int width, int height);
BGR_pixel **read_image_pixels           (int width, int height, FILE *inputFile, fpos_t position);
int       **alloc_pixels_memory         (int width, int height);
int         pixel_can_be_part_of_cluster(BGR_pixel pixel, BGR_pixel *inputPixRef, BGR_pixel *inputPixOffset);
void        reference_pixels_for_cluster(int width, int height, BGR_pixel **pixel, BGR_pixel *inputPixelRef, BGR_pixel *inputPixelOffset, int **referencePixels);
int         is_pixel_checked            (int width, int height, int **referencePixels, int row, int col, bool **isPixelChecked);
void        min_max                     (int *minRow, int *minColumn, int *maxRow, int *maxColumn, int row, int col);
void        found_cluster               (int width, int height, int **referencePixels, int row, int col, bool **isPixelChecked, int *nr_elem);
void        pixels_from_valid_cluster   (int width, int height,int **referencePixels, int row, int col, bool **pixelsFromValidCluster, 
                                        int *minRow, int *minColumn, int *maxRow, int *maxColumn);
int        *found_valid_clusters        (int width, int height, int **referencePixels, double clusterSizeFactor, int *nrClusters,
                                         bool **isPixelChecked, bool **pixelsFromValidCluster, int *minRow, int *minColumn, int *maxRow, int *maxColumn);
int         compare                     (const void * a, const void * b);
void        point_A                     (char *outputFilename, int nrClusters, int *foundCluster);
void        make_bmp                    (int width, int height, char *outputImage, struct bmp_fileheader *imageFileheader
                                        , struct bmp_infoheader *imageInfoheader, BGR_pixel **pixel);
void        copyimage_bitmap_data       (int width, int height, BGR_pixel **image, BGR_pixel **copyImage);
void        average                     (BGR_pixel **image, BGR_pixel **copyImage, int row, int col, int width, int height);
void        blur_image                  (int width, int height, BGR_pixel **copyImageImage, bool **isPixelChecked);
void        crop_header                 (struct bmp_fileheader *imageFileheader, struct bmp_infoheader *imageInfoheader,
                                        struct bmp_fileheader **cropImageFileheader, struct bmp_infoheader **cropImageInfoheader,
                                        int minRow, int minColumn, int maxRow, int maxColumn);
char      **output_filename             (int nrClusters);
void        crop_image                  (char *filename, int minRow, int minColumn, int maxRow, int maxColumn, struct bmp_fileheader *cropImageFileheader,
                                        struct bmp_infoheader *cropImageInfoheader, BGR_pixel **image_pixel);

int main(int argc, char const *argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    
    BGR_pixel *inputPixelRef = malloc(sizeof(BGR_pixel));
    BGR_pixel *inputPixelOffset = malloc(sizeof(BGR_pixel));
    double clusterSizeFactor;

    read_input_pixels(inputPixelRef, inputPixelOffset, &clusterSizeFactor, "input.txt");
    
    FILE *inputFile = fopen("input.bmp", "rb");

    if (!inputFile) {
        printf("Error! Can't open input.bmp\n");
        return -1;
    }

    struct bmp_fileheader *imageFileheader = malloc(sizeof(struct bmp_fileheader));
    struct bmp_infoheader *imageInfoheader = malloc(sizeof(struct bmp_infoheader));
    fpos_t lastReadPositionInputfile;

    read_header(imageFileheader, imageInfoheader, &lastReadPositionInputfile, inputFile);

    int width = imageInfoheader->width;
    int height = imageInfoheader->height;
    BGR_pixel **imagePixels = read_image_pixels(width, height, inputFile, lastReadPositionInputfile);
    
    bool **isPixelChecked = malloc(height * sizeof(bool*));
    for (int i = 0; i < height; i++) {
        isPixelChecked[i] = calloc(width, sizeof(bool));
    }

    bool **pixelsFromValidCluster = malloc(height * sizeof(bool*)); //pixels from valid clusters
    for (int i = 0; i < width; i++) {
        pixelsFromValidCluster[i] = calloc(height, sizeof(bool));
    }

    int **referencePixels = alloc_pixels_memory(width, height); //pixels that can be part of a cluster
    int nrClusters = 0;
    int *minRow = calloc(6, sizeof(int));
    int *minColumn = calloc(6, sizeof(int));
    int *maxRow = calloc(6, sizeof(int));
    int *maxColumn = calloc(6, sizeof(int));

    reference_pixels_for_cluster(width, height, imagePixels, inputPixelRef, inputPixelOffset, referencePixels);
    int *foundCluster = found_valid_clusters(width, height, referencePixels, clusterSizeFactor, &nrClusters, isPixelChecked, 
                        pixelsFromValidCluster, minRow, minColumn, maxRow, maxColumn);

    point_A("output.txt", nrClusters, foundCluster);

    //pct B
    BGR_pixel **copyImage = alloc_image_pixels(width, height);
    copyimage_bitmap_data(width, height, imagePixels, copyImage);
    blur_image(width, height, copyImage, pixelsFromValidCluster);
    make_bmp(width, height, "output_blur.bmp", imageFileheader, imageInfoheader, copyImage);

    // pct C
    struct bmp_fileheader **cropImageFileheader = malloc(nrClusters * sizeof(struct bmp_fileheader *));
    struct bmp_infoheader **cropImageInfoheader = malloc(nrClusters * sizeof(struct bmp_infoheader *));
    char **filename = output_filename(nrClusters);
    
    for (int i = 0; i < nrClusters; i++) {
        crop_header(imageFileheader, imageInfoheader, &cropImageFileheader[i], &cropImageInfoheader[i], minRow[i], minColumn[i], maxRow[i], maxColumn[i]);
        crop_image(filename[i], minRow[i], minColumn[i], maxRow[i], maxColumn[i], cropImageFileheader[i], cropImageInfoheader[i], imagePixels);
    }

    free(inputPixelRef);
    free(inputPixelOffset);
    free(imageInfoheader);
    free(imageFileheader);
    for (int i = 0; i < height; i++) {
        free(referencePixels[i]);
    }
    free(referencePixels);
    free(foundCluster);
    for (int i = 0; i < height; i++) {
        free(copyImage[i]);
    }
    free(copyImage);
    for (int i = 0; i < height; i++) {
        free(imagePixels[i]);
    }
    free(imagePixels);
    free(minRow);
    free(minColumn);
    free(maxRow);
    free(maxColumn);
    for (int i = 0; i < nrClusters; i++) {
        free(filename[i]);
    }
    free(filename);
    fclose(inputFile);

    return 0;
}

void read_pixel(BGR_pixel *pixel, FILE *inputFile) {
    fscanf(inputFile, "%hhu", &(*pixel).R);
    fscanf(inputFile, "%hhu", &(*pixel).G);
    fscanf(inputFile, "%hhu", &(*pixel).B);
}

void read_input_pixels(BGR_pixel *ref, BGR_pixel *offset, double *clusterSizeFactor, char *inputFilename) {
    FILE *inputFile = fopen(inputFilename, "rt");
    if (!inputFile) {
        fprintf(stderr, "ERROR: Can't open input file");
    }

    read_pixel(ref, inputFile);
    read_pixel(offset, inputFile);
    fscanf(inputFile, "%lf", clusterSizeFactor);
    fclose(inputFile);
}

// TODO: define macro suport debugging and uncomment the printf calls
void read_header(struct bmp_fileheader *imageFileheader, struct bmp_infoheader *imageInfoheader, fpos_t *position, FILE *inputFile) {

    fread(imageFileheader, sizeof(struct bmp_fileheader), 1, inputFile);
    // printf("File marker:%c%c\n", (*imageFileheader)->fileMarker1, (*imageFileheader)->fileMarker2);
    // printf("bfSize: %u\n", (*imageFileheader)->bfSize);
    // printf("unused1: %d unused2: %d\n", (*imageFileheader)->unused1, (*imageFileheader)->unused2);
    // printf("imageDataOffset:%d\n", (*imageFileheader)->imageDataOffset);

    fread(imageInfoheader, sizeof(struct bmp_infoheader), 1, inputFile);
    imageInfoheader->biXPelsPerMeter = 0;
    imageInfoheader->biYPelsPerMeter = 0;
    // printf("biSize:%d\n", (*imageInfoheader)->biSize);
    // printf("width:%d height: %d\n", (*imageInfoheader)->width, (*imageInfoheader)->height);
    // printf("planes: %d\n", (*imageInfoheader)->planes);
    // printf("bitPix: %d\n", (*imageInfoheader)->bitPix);
    // printf("biCompression: %d\n", (*imageInfoheader)->biCompression);
    // printf("biSizeImage: %d\n", (*imageInfoheader)->biSizeImage);
    // printf("biX: %d biY: %d\n", (*imageInfoheader)->biXPelsPerMeter, (*imageInfoheader)->biYPelsPerMeter);
    // printf("biClrUsed: %d\n", (*imageInfoheader)->biClrUsed);
    // printf("biClrImportant: %d\n", (*imageInfoheader)->biClrImportant);

    fgetpos(inputFile, position);
}

BGR_pixel **alloc_image_pixels(int width, int height) {
    BGR_pixel **imagePixels = malloc(height * sizeof(BGR_pixel *));
    for (int row = 0; row < height; row++) {
        imagePixels[row] = calloc(width, sizeof(BGR_pixel));
    }

    return imagePixels;
}

BGR_pixel **read_image_pixels(int width, int height, FILE *inputFile, fpos_t position) {
    BGR_pixel **imagePixels = alloc_image_pixels(width, height);

    if (imagePixels == NULL) {
        printf("[ERROR] Can't alloc memory for image pixels!\n");
        return NULL;
    }

    fsetpos(inputFile, &position);

    int padding = 4-((sizeof(BGR_pixel) * width)%4);
    if (padding == 4) {
        padding = 0;
    }

    int i, j;
    for(i = 0; i < height; ++i) {
        for(j = 0; j < width; ++j) {
            fread(&(imagePixels[i][j]), sizeof(BGR_pixel), 1, inputFile);
        }

        fseek(inputFile, padding, SEEK_CUR);
        if (padding < 4) {
            for (int k = j+1; k <= j+padding; k++) {
                imagePixels[i][k].B = 0;
                imagePixels[i][k].G = 0;
                imagePixels[i][k].R = 0;
            }
        }
    }

    return imagePixels;
}

int **alloc_pixels_memory(int width, int height) {
    int **pixelsMatrix = malloc(height * sizeof(int *));
    for (int i = 0; i < height; i++) {
        pixelsMatrix[i] = calloc(width, sizeof(int));
    }

    return pixelsMatrix;
}

int pixel_can_be_part_of_cluster(BGR_pixel pixel, BGR_pixel *inputPixRef, BGR_pixel *inputPixOffset) {
    if ((pixel.B < (inputPixRef->B - inputPixOffset->B)) || (pixel.B > (inputPixRef->B + inputPixOffset->B)))
        return 0;
    if ((pixel.G < (inputPixRef->G - inputPixOffset->G)) || (pixel.G > (inputPixRef->G + inputPixOffset->G)))
        return 0;
    if ((pixel.R < (inputPixRef->R - inputPixOffset->R)) || (pixel.R > (inputPixRef->R + inputPixOffset->R)))
        return 0;

    return 1;
}

//fill a matrix with 1 if a pixel can be part of a cluster
void reference_pixels_for_cluster(int width, int height, BGR_pixel **pixel, BGR_pixel *inputPixelRef, BGR_pixel *inputPixelOffset, int **referencePixels) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (pixel_can_be_part_of_cluster(pixel[i][j], inputPixelRef, inputPixelOffset)) {
                referencePixels[i][j] = 1;;
            } else {
                referencePixels[i][j] = 0;
            }
        }
    }
}

//check if a pixel is still in the limits of the image and is not part of another cluster
int is_pixel_checked(int width, int height, int **referencePixels, int row, int col, bool **isPixelChecked)  { 
    return (row >= 0) && (row < height) && (col >= 0) && (col < width) && (referencePixels[row][col] && !isPixelChecked[row][col]); 
} 

// TODO: refactor to ca more generic function and without side effects
//find the min and max for row and column of each valid cluster
void min_max(int *minRow, int *minColumn, int *maxRow, int *maxColumn, int row, int col) {
    if (*minRow > row) {
        *minRow = row;
    } else if (*maxRow < row){
        *maxRow = row;
    }

    if (*minColumn > col) {
        *minColumn = col;
    } else if (*maxColumn < col){
        *maxColumn = col;
    }
}

//found clusters that have pixels in the range of reference pixel and offset pixels
void found_cluster(int width, int height, int **referencePixels, int row, int col, bool **isPixelChecked, int *nr_elem) { 
    // arrays for 4 neighbours 
    static int rowNbr[] = {0, 1, 0, -1}; 
    static int colNbr[] = {1, 0, -1, 0}; 

    // Mark this pixel as checked 
    isPixelChecked[row][col] = true; 
    
    //check the 4 neighbours of a pixel
    for (int k = 0; k < 4; ++k) {
        if (is_pixel_checked(width, height, referencePixels, row + rowNbr[k], col + colNbr[k], isPixelChecked)) {
            (*nr_elem)++;
            found_cluster(width, height, referencePixels, row + rowNbr[k], col + colNbr[k], isPixelChecked, nr_elem); 
        }
    }
}

//create a map for the valid clusters  
void pixels_from_valid_cluster(int width, int height,int **referencePixels, int row, int col, bool **pixelsFromValidCluster, 
                                int *minRow, int *minColumn, int *maxRow, int *maxColumn) { 
    // arrays for 4 neighbours 
    static int rowNbr[] = {0, 1, 0, -1}; 
    static int colNbr[] = {1, 0, -1, 0}; 

    // Mark this pixel as checked
    pixelsFromValidCluster[row][col] = true; 
    
    for (int k = 0; k < 4; ++k) {
        if (is_pixel_checked(width, height, referencePixels, row + rowNbr[k], col + colNbr[k], pixelsFromValidCluster)) {
            min_max(minRow, minColumn, maxRow, maxColumn, row + rowNbr[k], col + colNbr[k]);
            pixels_from_valid_cluster(width, height, referencePixels, row + rowNbr[k], col + colNbr[k], pixelsFromValidCluster, minRow, minColumn, maxRow, maxColumn); 
        }
    }
}  

//found the clusters that has the nr of pixels bigger of equal with the minimum nr of pixel given by equation: clusterSizeFactor*width*height
int *found_valid_clusters(int width, int height, int **referencePixels, double clusterSizeFactor, int *nrClusters
    , bool **isPixelChecked, bool **pixelsFromValidCluster, int *minRow, int *minColumn, int *maxRow, int *maxColumn) {   
    int sizeFoundClusters = 2, indexCluster = 0;
    int *foundClusters = calloc(sizeFoundClusters, sizeof(int));
    int minNrOfPixels = clusterSizeFactor * width * height; 
    int nrElem = 0;
    

    for (int i = 0; i < height; ++i){ 
        for (int j = 0; j < width; ++j){ 
            if (!referencePixels[i][j] || isPixelChecked[i][j]) {
                continue;
            } 
            nrElem = 1;
            found_cluster(width, height, referencePixels, i, j, isPixelChecked, &nrElem); // Visit all cells in this cluster.

            if (nrElem >= minNrOfPixels) { //if nr of element is bigger than minNrOfPixels the cluster is valid
                //found the min and max for row and column for each valid cluster
                minRow[indexCluster] = i;
                minColumn[indexCluster] = j;
                maxRow[indexCluster] = 0;
                maxColumn[indexCluster] = 0;

                pixels_from_valid_cluster(width, height, referencePixels, i, j, pixelsFromValidCluster, &minRow[indexCluster], 
                                          &minColumn[indexCluster], &maxRow[indexCluster], &maxColumn[indexCluster]);
                
                if (sizeFoundClusters == indexCluster+1) {
                    sizeFoundClusters *= 2;
                    foundClusters = realloc(foundClusters, sizeFoundClusters * sizeof(int));
                }

                (*nrClusters)++;
                foundClusters[indexCluster] = nrElem;
                indexCluster++;
            } 
        }
    }
    return foundClusters;
} 

int compare (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}

void point_A(char *outputFilename, int nrClusters, int *foundCluster) {
    FILE *outputFile = fopen(outputFilename, "wt");
    if (!outputFile) {
        printf("Error at opening output file\n");
    }

    if (nrClusters > 1) {
     qsort(foundCluster, nrClusters, sizeof(int), compare);
    }

    for (int i = 0; i < nrClusters; i++) {
        fprintf(outputFile, "%d ", foundCluster[i]);
    }

    fclose(outputFile);
}

void make_bmp(int width, int height, char *outputImage, struct bmp_fileheader *imageFileheader
                , struct bmp_infoheader *imageInfoheader, BGR_pixel **pixel) {
    FILE *image = fopen(outputImage, "wb");
    if (!image) {
        printf("Error\n");
    }

    fwrite(imageFileheader, sizeof(struct bmp_fileheader), 1, image);
    fwrite(imageInfoheader, sizeof(struct bmp_infoheader), 1, image);

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            fwrite(&pixel[i][j], sizeof(BGR_pixel), 1, image);
        }
    }

    fclose(image);
}

//create a copyImage of the pixels of an image
void copyimage_bitmap_data(int width, int height, BGR_pixel **image, BGR_pixel **copyImage) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            copyImage[i][j].B = 0;
            copyImage[i][j].G = 0;
            copyImage[i][j].R = 0;
        }
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            copyImage[i][j] = image[i][j];
        }
    }
}

void average(BGR_pixel **image, BGR_pixel **copyImage, int row, int col, int width, int height) {
    if (row == 0 && col == 0) {
        image[row][col].B = (copyImage[row+1][col].B + copyImage[row][col+1].B) / 2;
        image[row][col].G = (copyImage[row+1][col].G + copyImage[row][col+1].G) / 2;
        image[row][col].R = (copyImage[row+1][col].R + copyImage[row][col+1].R) / 2;
    } else if (row == 0 && col == width -1) {
        image[row][col].B = (copyImage[row+1][col].B + copyImage[row][col-1].B) / 2;
        image[row][col].G = (copyImage[row+1][col].G + copyImage[row][col-1].G) / 2;
        image[row][col].R = (copyImage[row+1][col].R + copyImage[row][col-1].R) / 2;
    } else if (row == height-1 && col == 0 ) {
        image[row][col].B = (copyImage[row-1][col].B + copyImage[row][col+1].B) / 2;
        image[row][col].G = (copyImage[row-1][col].G + copyImage[row][col+1].G) / 2;
        image[row][col].R = (copyImage[row-1][col].R + copyImage[row][col+1].R) / 2;
    } else if (row == height-1 && col == width-1) {
        image[row][col].B = (copyImage[row-1][col].B + copyImage[row][col-1].B) / 2;
        image[row][col].G = (copyImage[row-1][col].G + copyImage[row][col-1].G) / 2;
        image[row][col].R = (copyImage[row-1][col].R + copyImage[row][col-1].R) / 2;
    } else if (row == 0) {
        image[row][col].B = (copyImage[row+1][col].B + copyImage[row][col+1].B + copyImage[row][col-1].B) / 3;
        image[row][col].G = (copyImage[row+1][col].G + copyImage[row][col+1].G + copyImage[row][col-1].G) / 3;
        image[row][col].R = (copyImage[row+1][col].R + copyImage[row][col+1].R + copyImage[row][col-1].R) / 3;
    } else if (row == height - 1) {
        image[row][col].B = (copyImage[row-1][col].B + copyImage[row][col+1].B + copyImage[row][col-1].B) / 3;
        image[row][col].G = (copyImage[row-1][col].G + copyImage[row][col+1].G + copyImage[row][col-1].G) / 3;
        image[row][col].R = (copyImage[row-1][col].R + copyImage[row][col+1].R + copyImage[row][col-1].R) / 3;
    } else if (col == 0) {
        image[row][col].B = (copyImage[row+1][col].B + copyImage[row][col+1].B + copyImage[row-1][col].B) / 3;
        image[row][col].G = (copyImage[row+1][col].G + copyImage[row][col+1].G + copyImage[row-1][col].G) / 3;
        image[row][col].R = (copyImage[row+1][col].R + copyImage[row][col+1].R + copyImage[row-1][col].R) / 3;
    } else if (col == width-1) {
        image[row][col].B = (copyImage[row+1][col].B + copyImage[row-1][col].B + copyImage[row][col-1].B) / 3;
        image[row][col].G = (copyImage[row+1][col].G + copyImage[row-1][col].G + copyImage[row][col-1].G) / 3;
        image[row][col].R = (copyImage[row+1][col].R + copyImage[row-1][col].R + copyImage[row][col-1].R) / 3;
    } else {
        image[row][col].B = (copyImage[row+1][col].B + copyImage[row][col+1].B + copyImage[row][col-1].B + copyImage[row-1][col].B) / 4;
        image[row][col].G = (copyImage[row+1][col].G + copyImage[row][col+1].G + copyImage[row][col-1].G + copyImage[row-1][col].G) / 4;
        image[row][col].R = (copyImage[row+1][col].R + copyImage[row][col+1].R + copyImage[row][col-1].R + copyImage[row-1][col].R) / 4;
    }
}

void blur_image(int width, int height, BGR_pixel **copyImageImage, bool **isPixelChecked){
    //making another copyImage of the original image because it has not to be modify
    BGR_pixel **secondcopyImage = alloc_image_pixels(width, height);
    copyimage_bitmap_data(width, height, copyImageImage, secondcopyImage);

    for (int iteration = 0; iteration < 100; iteration++) {
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                if (isPixelChecked[row][col] == true) {
                    average(copyImageImage, secondcopyImage, row, col, width, height);
                } 
            }
        }
        copyimage_bitmap_data(width, height, copyImageImage, secondcopyImage); //after every blur create a copyImage of the blur image
    }
    
    for (int i = 0; i < height; i++) {
        free(secondcopyImage[i]);
    }
    free(secondcopyImage);
}

void crop_header(struct bmp_fileheader *imageFileheader, struct bmp_infoheader *imageInfoheader,
    struct bmp_fileheader **cropImageFileheader, struct bmp_infoheader **cropImageInfoheader,
    int minRow, int minColumn, int maxRow, int maxColumn) {

    *cropImageFileheader = calloc(1, sizeof(struct bmp_fileheader));
    *cropImageInfoheader = calloc(1, sizeof(struct bmp_infoheader));

    (*cropImageInfoheader)->biSize =sizeof(struct bmp_infoheader);
    (*cropImageInfoheader)->width = maxColumn - minColumn + 1;
    (*cropImageInfoheader)->height = maxRow - minRow + 1;
    (*cropImageInfoheader)->planes = imageInfoheader->planes;
    (*cropImageInfoheader)->bitPix = imageInfoheader->bitPix;
    (*cropImageInfoheader)->biCompression = imageInfoheader->biCompression;
    int padding = 4 - ((sizeof(BGR_pixel) * (*cropImageInfoheader)->width)%4);
    if (padding < 4) {
        (*cropImageInfoheader)->biSizeImage = ((*cropImageInfoheader)->width * (*cropImageInfoheader)->height) * sizeof(BGR_pixel) + 
        (padding * (*cropImageInfoheader)->height);
        
    } else {
        (*cropImageInfoheader)->biSizeImage = ((*cropImageInfoheader)->width * (*cropImageInfoheader)->height) * sizeof(BGR_pixel);
    }

    (*cropImageInfoheader)->biXPelsPerMeter = 0;
    (*cropImageInfoheader)->biYPelsPerMeter = 0;
    (*cropImageInfoheader)->biClrUsed = 0;
    (*cropImageInfoheader)->biClrImportant = 0;

    (*cropImageFileheader)->fileMarker1 = imageFileheader->fileMarker1;
    (*cropImageFileheader)->fileMarker2 = imageFileheader->fileMarker2;
    (*cropImageFileheader)->bfSize = (*cropImageInfoheader)->biSizeImage + sizeof(struct bmp_infoheader) + sizeof(struct bmp_fileheader);
    (*cropImageFileheader)->unused1 = imageFileheader->unused1;
    (*cropImageFileheader)->unused2 = imageFileheader->unused2;
    (*cropImageFileheader)->imageDataOffset = sizeof(struct bmp_infoheader) + sizeof(struct bmp_fileheader);

}

char **output_filename(int nrClusters) {
    char **filename = malloc(nrClusters * sizeof (char *));

    for (int i = 0; i < nrClusters; i++) {
        filename[i] = calloc(20, sizeof(char));
    }
    for (int i = 0; i < nrClusters; i++) {
        snprintf(filename[i], 20, "output_crop%d.bmp", i+1);
        printf("name: %s\n", filename[i]);
    }

    return filename;
}

void crop_image(char *filename, int minRow, int minColumn, int maxRow, int maxColumn, struct bmp_fileheader *cropImageFileheader,
                struct bmp_infoheader *cropImageInfoheader, BGR_pixel **image_pixel) {
    unsigned char pixel = 0;
    int padding = 4 - ((sizeof(BGR_pixel) * cropImageInfoheader->width)%4);
    FILE *im = fopen(filename, "w+");

    fwrite(cropImageFileheader, sizeof(struct bmp_fileheader), 1, im);
    fwrite(cropImageInfoheader, sizeof(struct bmp_infoheader), 1, im);
    for (int i = minRow; i <= maxRow; i++) {
        for (int j = minColumn; j <= maxColumn; j++) {
            fwrite(&image_pixel[i][j], sizeof(BGR_pixel), 1, im);
            if (j == maxColumn && padding < 4) {
                for (int i = 0; i < padding; i++) {
                    fwrite(&pixel, sizeof(unsigned char), 1, im);
                }
            }
        }
    }
    fclose(im);
}