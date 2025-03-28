#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgcodec/imgcodec_c.h>

#prama comment(lib, "cv.lib")
#prama comment(lib, "highgui.lib")
#prama comment(lib, "cxcore.lib")

int main(int argc, char **argv)
{
	IplImage *image1 = cvLoadImage("lenna.png", CV_LOAD_IMAGE_COLOR);
	IplImage *image2 = cvLoadImage("mandrill.png", CV_LOAD_IMAGE_COLOR);
	IplImage *image_add = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,3);
	IplImage *image_sub = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,3);
	IplImage *image_mul = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,3);
	IplImage *image_div = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,3);
	IplImage *image_gray1 = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,1);
	IplImage *image_gray2 = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,1);
	IplImage *image_white = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,1);
	IplImage *image_gray_sub = cvCreateImage(cvGetSize(image1), IPL_DEPTH_8U,1);

	cvNameWidow("IMAGE_1",CV_WINDOW_AUTOSIZE);
	cvNameWidow("IMAGE_2",CV_WINDOW_AUTOSIZE);
	cvNameWidow("IMAGE_ADDITION",CV_WINDOW_AUTOSIZE);
	cvNameWidow("IMAGE_SUBTRACTION",CV_WINDOW_AUTOSIZE);
	cvNameWidow("IMAGE_MULTIPLICATION",CV_WINDOW_AUTOSIZE);
	cvNameWidow("IMAGE_DIVISION",CV_WINDOW_AUTOSIZE);
	cvNameWidow("IMAGE_GRAY1",CV_WINDOW_AUTOSIZE);
	cvNameWidow("IMAGE_GRAY2",CV_WINDOW_AUTOSIZE);
	cvNameWidow("IMAGE_WHITE",CV_WINDOW_AUTOSIZE);

	cvAdd(image1, image2, image_add,NULL);
	cvSub(image1, image2, image_sub,NULL);
	cvMul(image1, image2, image_mul,NULL);
	cvDiv(image1, image2, image_div,NULL);
	cvCvtColor(image1, image_gray1, image_add,CV_RGB2GRAY);
	cvCvtColor(image2, image_gray2, image_add,CV_RGB2GRAY);
	cvAbsDiff(image_gray1, image_gray2, image_gray_sub);
	cvThreshold(image_gray_sub, image_white, 100,255,CV_THRESH_BINARY);

	cvShowImage("IMAGE_1",image1);
	cvShowImage("IMAGE_2",image2);
	cvShowImage("IMAGE_ADDTION",image_add);
	cvShowImage("IMAGE_SUBTRACTION",image_sub);
	cvShowImage("IMAGE_MULTIPLICATION",image_mul);
	cvShowImage("IMAGE_DIVISION",image_div);
	cvShowImage("IMAGE_GRAY1",image_gray1);
	cvShowImage("IMAGE_GRAY2",image_gray2);
	cvShowImage("IMAGE_WHITE",image_white);
	
	cvWaitKey(0);

	cvReleaseImage(&image1);
	cvReleaseImage(&image2);
	cvReleaseImage(&image_add);
	cvReleaseImage(&image_sub);
	cvReleaseImage(&image_mul);
	cvReleaseImage(&image_div);
	cvReleaseImage(&image_gray1);
	cvReleaseImage(&image_gray2);
	cvReleaseImage(&image_white);
	cvReleaseImage(&image_gray_sub);

	cvDestroyWindows();

	return 0;
}


