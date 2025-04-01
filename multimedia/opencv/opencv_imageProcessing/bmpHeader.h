#ifndef __BMP_FILE_H__  // Include guard to prevent multiple inclusion of the same header file.
#define __BMP_FILE_H__

/**
 * BITMAPFILEHEADER:
 * This structure defines the file header of a BMP file. It contains metadata 
 * about the file such as the file type, size, and the offset where the pixel data starts.
 */
typedef struct __attribute__((__packed__)) // The __packed__ attribute is used to prevent padding, ensuring that the structure is tightly packed.
{
    unsigned short bfType;          // Specifies the file type; must be 'BM' for BMP files (0x4D42 in hexadecimal).
    unsigned int bfSize;            // Size of the BMP file in bytes.
    unsigned short bfReserved1;     // Reserved; must be set to 0.
    unsigned short bfReserved2;     // Reserved; must be set to 0.
    unsigned int bfOffBits;         // Offset from the beginning of the file to the pixel data (bitmap bits).
} BITMAPFILEHEADER;

/**
 * BITMAPINFOHEADER:
 * This structure defines the image header, which contains detailed information 
 * about the bitmap image such as dimensions, color depth, and compression.
 */
typedef struct {
    unsigned int biSize;            // Size of this header (in bytes), typically 40 bytes.
    unsigned int biWidth;           // Width of the image in pixels.
    unsigned int biHeight;          // Height of the image in pixels.
    unsigned short biPlanes;        // Number of color planes, must be 1.
    unsigned short biBitCount;      // Number of bits per pixel (color depth), typically 1, 4, 8, 16, 24, or 32.
    unsigned int biCompression;     // Compression method used (0 = none, 1 = RLE-8, 2 = RLE-4, etc.).
    unsigned int SizeImage;         // Size of the image data in bytes (may be 0 if no compression is used).
    unsigned int biXPelsPerMeter;   // Horizontal resolution in pixels per meter.
    unsigned int biYPelsPerMeter;   // Vertical resolution in pixels per meter.
    unsigned int biClrUsed;         // Number of colors used in the bitmap (0 means default for the color depth).
    unsigned int biClrImportant;    // Number of important colors (0 means all are important).
} BITMAPINFOHEADER;

/**
 * RGBQUAD:
 * This structure defines the color table entries for the bitmap (used in 1, 4, or 8-bit images).
 * Each entry contains the RGB (red, green, blue) color values and a reserved byte.
 */
typedef struct
{
    unsigned char rgbBlue;      // Blue intensity value (0-255).
    unsigned char rgbGreen;     // Green intensity value (0-255).
    unsigned char rgbRed;       // Red intensity value (0-255).
    unsigned char rgbReserved;  // Reserved for future use; typically set to 0.
} RGBQUAD;

#endif  // End of the include guard.
