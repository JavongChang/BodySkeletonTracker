#include "SampleViewer.h"

#include <math.h>
#include <string.h>

#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>

using namespace cv;
using namespace std;

#ifdef DEPTH
#define SAMPLE_READ_WAIT_TIMEOUT 2000 //2000ms
using namespace openni;
#endif

SampleViewer* SampleViewer::ms_self = NULL;

SampleViewer::SampleViewer(const char* strSampleName, const char* deviceUri)
{
#ifdef DEPTH
	initOpenNI(deviceUri);
	
	skelD = NULL;
#else
    if( deviceUri==NULL )
    {
        int camera = 0;
        if(!capture.open(camera))
            cout << "Capture from camera #" <<  camera << " didn't work" << endl;
    }
    else
    {
        Mat image = imread( deviceUri, 1 );
        if( image.empty() )
        {
            if(!capture.open( deviceUri ))
                cout << "Could not read " << deviceUri << endl;
        }
    }
#endif
	ms_self = this;
	strncpy(m_strSampleName, strSampleName, strlen(strSampleName));

	skel = NULL;
	subSample = 2;
	frameCount = 0;
}


int SampleViewer::initOpenNI(const char* deviceUri) {
	Status rc = OpenNI::initialize();
	if (rc != STATUS_OK)
	{
		printf("Initialize failed\n%s\n", OpenNI::getExtendedError());
		return 1;
	}

	rc = device.open(deviceUri);
	if (rc != STATUS_OK)
	{
		printf("Couldn't open device\n%s\n", OpenNI::getExtendedError());
		return 2;
	}

	if (device.getSensorInfo(SENSOR_DEPTH) != NULL)
	{
		rc = depth.create(device, SENSOR_DEPTH);
		if (rc != STATUS_OK)
		{
			printf("Couldn't create depth stream\n%s\n", OpenNI::getExtendedError());
			return 3;
		}
	}

	rc = depth.start();
	if (rc != STATUS_OK)
	{
		printf("Couldn't start the depth stream\n%s\n", OpenNI::getExtendedError());
		return 4;
	}
}


/**
 * Ler o proximo frame e o retorna.
 **/
VideoFrameRef * SampleViewer::getNextFrame() {
	int changedStreamDummy;
	VideoStream* pStream = &depth;
	int rc = OpenNI::waitForAnyStream(&pStream, 1, &changedStreamDummy, SAMPLE_READ_WAIT_TIMEOUT);
	if (rc != STATUS_OK)
	{
		printf("Wait failed! (timeout is %d ms)\n%s\n", SAMPLE_READ_WAIT_TIMEOUT, OpenNI::getExtendedError());
		return NULL;
	}
	
	openni::VideoFrameRef *frame = new VideoFrameRef();
	rc = depth.readFrame(frame);
	if (rc != STATUS_OK)
	{
		printf("Read failed!\n%s\n", OpenNI::getExtendedError());
		return NULL;
	}

	if (frame->getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_1_MM && frame->getVideoMode().getPixelFormat() != PIXEL_FORMAT_DEPTH_100_UM)
	{
		printf("Unexpected frame format\n");
		return NULL;
	}

	return frame;
}

Point3D* SampleViewer::getClosestPoint(openni::VideoFrameRef *frame) {
	Point3D *closestPoint = new Point3D();
	DepthPixel* pDepth = (DepthPixel*)frame->getData();
	bool found = false;
	closestPoint->z = 0xffff;
	int width = frame->getWidth();
	int height = frame->getHeight();

	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x, ++pDepth)
		{
			if (*pDepth < closestPoint->z && *pDepth != 0)
			{
				closestPoint->x = x;
				closestPoint->y = y;
				closestPoint->z = *pDepth;
				found = true;
			}
		}

	if (!found)
	{
		return NULL;
	}

	return closestPoint;
}

SampleViewer::~SampleViewer()
{
	finalize();

	if (m_pTexMap)
		delete[] m_pTexMap;

	ms_self = NULL;

	if (skel)
            delete skel;
#ifdef DEPTH
	if (skelD)
            delete skelD;
#endif
}

void SampleViewer::finalize()
{
#ifdef DEPTH
	depth.stop();
	depth.destroy();
	device.close();
	OpenNI::shutdown();
#endif
}

int SampleViewer::init()
{
	m_pTexMap = NULL;

	return 0;
}

int SampleViewer::run()	//Does not return
{
#ifdef DEPTH
printf("Compilado com Depth\n");
#else
printf("Compilado SEM Depth\n");
#endif

	while (1) {
		display();
		char c = (char)waitKey(10);
		//char c = (char)waitKey(100);
		//char c = (char)waitKey(200);
		//char c = (char)waitKey(500);
		//char c = (char)waitKey(1000);
	        if( c == 27 || c == 'q' || c == 'Q' )
        	        break;
	}
	//int r = system("killall -9 rostopic");
#ifdef DEPTH
	return openni::STATUS_OK;
#else
	return 0;
#endif
}
void SampleViewer::display()
{
	int sizePixel = 3;
#ifdef DEPTH
	openni::VideoFrameRef * srcFrame = getNextFrame();
	if (srcFrame==NULL)
		return;
	Point3D * closest = getClosestPoint(srcFrame);

#else
	Mat srcFrame;
	srcFrame.data = NULL;
	if( capture.isOpened() ) {
		Mat framec;
		capture >> framec;
		if(! framec.empty() ) {
			srcFrame = cv::Mat(framec.size(), CV_8UC1);
			cvtColor(framec, srcFrame, CV_RGB2GRAY);
			//srcFrame = framec.clone();
		}
		else
			printf("frame nulo\n");
	}
#endif




	// So entra nesses if na primeira vez
#ifdef DEPTH
	if (m_pTexMap == NULL)
	{
		// Texture map init
		width = srcFrame->getWidth();
		height = srcFrame->getHeight();
#else
	if (m_pTexMap == NULL && srcFrame->data!=NULL)
	{
		// TODO pegar da webcam opencv
		width = srcFrame.cols;
		height = srcFrame.rows;
#endif
//printf("w x h = %d x %d\n", width, height);
		m_pTexMap = new unsigned char[width * height * sizePixel];

		skel = new Skeleton(width, height, subSample);
#ifdef DEPTH
		skelD = new SkeletonDepth(width, height, subSample);
#endif
		cvNamedWindow("Skeleton Traker", CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
		//cvSetWindowProperty("Skeleton Traker", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
		//resizeWindow("Skeleton Traker", width*2, height*2);
		resizeWindow("Skeleton Traker", width, height);
	}

//printf("sizeof(openni::RGB888Pixel)=%ld\n", sizeof(openni::RGB888Pixel) );

	frameCount++;

	// check if we need to draw depth frame to texture
#ifdef DEPTH
	if (srcFrame->isValid())
	{
		Mat binarized(cv::Size(width/subSample, height/subSample), CV_8UC1, cv::Scalar(0));
		short depthMat[width*height*sizeof(short)];
		bzero(depthMat, width*height*sizeof(short));

		memset(m_pTexMap, 0, width*height*sizeof(openni::RGB888Pixel));

		skelD->prepareAnalisa(closest);
		//colore e obtem a imagem binarizada
		skelD->paintDepthCopy((openni::RGB888Pixel*)m_pTexMap, *srcFrame, binarized, depthMat);
		
		skel->setDepthMat(depthMat);

		// Converte o openni::VideoFrameRef (srcFrame) para um cv::Mat (frame)
		Mat frame = Mat(Size(width, height), CV_8UC3);
		memcpy(frame.data, m_pTexMap, width*height*sizePixel);
#else
	if( srcFrame.data != NULL )
	{
		Mat binarizedFirst(cv::Size(width/subSample, height/subSample), CV_8UC1, cv::Scalar(0));
		Mat binarized     (cv::Size(width/subSample, height/subSample), CV_8UC1, cv::Scalar(0));

		cv::resize(srcFrame, binarizedFirst, binarizedFirst.size());

		cv::threshold(binarizedFirst, binarized, 50, 255, cv::THRESH_BINARY_INV);
		

		Mat frame = srcFrame;
		//Mat frame = binarizedFirst;

		// mode webcam RGB, discard the first 10 frames, because they can be too white.
		if (frameCount>10) {
#endif

		Mat binarizedCp = binarized.clone();
		skel->detectBiggerRegion(binarized);

		Mat * skeleton = skel->thinning02(binarized);
		skel->analyse(skeleton);

		std::vector<Point3D> bdireito = skel->getSkeletonArm(skeleton, true);
		std::vector<Point3D> besquerdo= skel->getSkeletonArm(skeleton, false);

		skel->locateMainBodyPoints(binarizedCp);

		//skel->drawOverFrame(skeleton, frame);
		//skel->drawOverFrame(bdireito, frame);
		//skel->drawOverFrame(besquerdo, frame);
		//skel->drawOverFrame(&binarizedCp, frame);

		skel->drawMarkers(frame);
		skel->prepare(depthMat, closest);
		
		notifyListeners(skel->getSkeletonPoints(), skel->getAfa());

		if (skeleton)
			delete skeleton;
#ifndef DEPTH
		}
#endif
		//imshow("Skeleton Traker", *skeleton);
		imshow("Skeleton Traker", frame );
		//imshow("Skeleton Traker", binarized2 );
	}

	if (srcFrame)
		delete srcFrame;
	if (closest)
		delete closest;
}


void SampleViewer::notifyListeners(SkeletonPoints * sp, int afa) {
	for (std::vector<SkeletonListener*>::iterator it = listeners.begin(); it != listeners.end(); it++)
		(*it)->onEvent(sp, afa);
}

void SampleViewer::registerListener(SkeletonListener * listener) {
	listeners.push_back(listener);
}


