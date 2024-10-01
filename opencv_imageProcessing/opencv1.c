#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgcodec/imgcodec_c.h>

#prama comment(lib, "cv.lib")
#prama comment(lib, "highgui.lib")
#prama comment(lib, "cxcore.lib")

int main(int argc, char **argv)
{
	IplImage *image1 = cvLoadImage("sample1.jpg", CV_LOAD_IMAGE_COLOR);
	IplImage *image2 = cvLoadImage("sample2.jpg", CV_LOAD_IMAGE_COLOR);
	IplImage *image_add = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,3);
	IplImage *image_sub = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,3);
	IplImage *image_mul = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,3);
	IplImage *image_div = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,3);
	IplImage *image_gray1 = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,1);
	IplImage *image_gray2 = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,1);
	IplImage *image_white = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,1);
	IplImage *image_gray_sub = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,1);

	cvNameWidow("IMAGE_1", 
}


