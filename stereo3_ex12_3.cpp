#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <cctype>

using namespace std;
using namespace cv;
static void StereoCalib(const char* imageList, int nx, int ny, int useUncalibrated)
{
    int displayCorners = 1;
    int showUndistorted = 1;
    bool isVerticalStereo = false;	//좌우 또는 상하로 배열된 스테레오 영상 지원?

    const int maxScale = 1;
    const float squareSize = 1.f;	// 실제 정사각형의 크기.
    FILE* f = fopen(imageList, "rt");
    int i, j, lr, nframes, n = nx*ny, N = 0;
    vector<string> imageNames[2];
    vector<CvPoint3D32f> objectPoints;
    vector<CvPoint2D32f> points[2];
    vector<int> npoints;
    vector<uchar> active[2];
    vector<CvPoint2D32f> temp(n);
    Size imageSize(0,0);

    // 행렬과 벡터를 저장할 변수 생성
    double M1[3][3], M2[3][3], D1[5], D2[5];
    double R[3][3], T[3], E[3][3], F[3][3];
    Mat _M1(3, 3, CV_64F, M1 );
    Mat _M2(3, 3, CV_64F, M2 );
    Mat _D1(1, 5, CV_64F, D1 );
    Mat _D2(1, 5, CV_64F, D2 );
    Mat _R(3, 3, CV_64F, R );
    Mat _T(3, 1, CV_64F, T );
    Mat _E(3, 3, CV_64F, E );
    Mat _F(3, 3, CV_64F, F );

    if( displayCorners )
        namedWindow( "corners", 1 );

    // 체스판 영상의 목록을 읽는다.
    if( !f )
    {
        fprintf(stderr, "can not open file %s\n", imageList );
        return;
    }

    for(i=0;;i++)
    {
        char buf[1024];
        int count = 0, result=0;
        lr = i % 2;
        vector<CvPoint2D32f>& pts = points[lr];



        if( !fgets( buf, sizeof(buf)-3, f ))
            break;

        size_t len = strlen(buf);
        while( len > 0 && isspace(buf[len-1]))
            buf[--len] = '\0';
        if( buf[0] == '#')
            continue;

        //IplImage* img = cvLoadImage( buf, 0 );
        Mat img = imread(buf,0);

        if( img.empty() )
            break;

        imageSize = cvGetSize(img);
        imageNames[lr].push_back(buf);

        // 체스판 내부의 코너점을 찾는다.
        for( int s = 1; s <= maxScale ; s++ )
        {
            IplImage* timg = img;
            if( s > 1 )
            {
                timg = cvCreateImage(cvSize(img->width*s, img->height*s),
                                     img->depth, img->nChannels );
                cvResize( img, timg, CV_INTER_CUBIC );
            }
            result = cvFindChessboardCorners( timg, cvSize(nx, ny),
                                              &temp[0], &count,
                                              CV_CALIB_CB_ADAPTIVE_THRESH |
                                              CV_CALIB_CB_NORMALIZE_IMAGE);
            if( timg != img )
                cvReleaseImage( &timg );
            if( result || s == maxScale )
                for( j = 0; j < count; j++ )
                {
                    temp[j].x /= s;
                    temp[j].y /= s;
                }
            if( result )
                break;
        }

        if( displayCorners )
        {
            printf("%s\n", buf);
            IplImage* cimg = cvCreateImage( imageSize, 8, 3 );
            cvCvtColor( img, cimg, CV_GRAY2BGR );
            cvDrawChessboardCorners( cimg, cvSize(nx, ny), &temp[0],
                                     count, result );
            cvShowImage( "corners", cimg );
            cvReleaseImage( &cimg );
            if( cvWaitKey(0) == 27 ) // ESC 키를 누르면 종료합니다.
                exit(-1);
        }
        else
            putchar('.');
        N = pts.size();
        pts.resize(N + n, cvPoint2D32f(0,0));
        active[lr].push_back((uchar)result);

        // assert( result != 0 );
        if( result )
        {
            // �����ȼ� ������ ���� ������ ������ ��Ȯ���� �ʴ�.
            cvFindCornerSubPix( img, &temp[0], count,
                                cvSize(11, 11), cvSize(-1,-1),
                                cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS,
                                               30, 0.01) );
            copy( temp.begin(), temp.end(), pts.begin() + N );
        }
        cvReleaseImage( &img );
    }
    fclose(f);
    printf("\n");
//
//    // ü������ 3D ��ü �� ����
//    nframes = active[0].size();
//    objectPoints.resize(nframes*n);
//    for( i = 0; i < ny; i++ )
//        for( j = 0; j < nx; j++ )
//            objectPoints[i*nx + j] =
//                    cvPoint3D32f(i*squareSize, j*squareSize, 0);
//    for( i = 1; i < nframes; i++ )
//        copy( objectPoints.begin(), objectPoints.begin() + n,
//              objectPoints.begin() + i*n );
//
//    npoints.resize(nframes,n);
//    N = nframes*n;
//
//    CvMat _objectPoints = cvMat(1, N, CV_32FC3, &objectPoints[0] );
//    CvMat _imagePoints1 = cvMat(1, N, CV_32FC2, &points[0][0] );
//    CvMat _imagePoints2 = cvMat(1, N, CV_32FC2, &points[1][0] );
//    CvMat _npoints = cvMat(1, npoints.size(), CV_32S, &npoints[0] );
//    cvSetIdentity(&_M1);
//    cvSetIdentity(&_M2);
//    cvZero(&_D1);
//    cvZero(&_D2);
//
//    // ���׷��� ī�޶� ����
//    printf("Running stereo calibration ...");
//    fflush(stdout);
//    cvStereoCalibrate( &_objectPoints, &_imagePoints1,
//                       &_imagePoints2, &_npoints,
//                       &_M1, &_D1, &_M2, &_D2,
//                       imageSize, &_R, &_T, &_E, &_F,
//                       15,
////                       cvTermCriteria(CV_TERMCRIT_ITER+
////                                      CV_TERMCRIT_EPS, 100, 1e-5),
//                       CV_CALIB_FIX_ASPECT_RATIO +
//                       CV_CALIB_ZERO_TANGENT_DIST +
//                       CV_CALIB_SAME_FOCAL_LENGTH );
//    printf(" done\n");
//
//    // ���� ǰ�� �˻�:
//    // ���� �⺻ ������ ���ǻ� ���� ���� ������ �����ϱ� ������
//    // �������� ���� ������ �̿��Ͽ� ���� ǰ���� �˻��� �� �ִ�.
//    // �������� ���� ����: m2^t*F*m1=0
//    vector<CvPoint3D32f> lines[2];
//    points[0].resize(N);
//    points[1].resize(N);
//    _imagePoints1 = cvMat(1, N, CV_32FC2, &points[0][0] );
//    _imagePoints2 = cvMat(1, N, CV_32FC2, &points[1][0] );
//    lines[0].resize(N);
//    lines[1].resize(N);
//    CvMat _L1 = cvMat(1, N, CV_32FC3, &lines[0][0]);
//    CvMat _L2 = cvMat(1, N, CV_32FC3, &lines[1][0]);
//
//    // �ְ��� ���ŵ� ���¿��� �����Ѵ�.
//    cvUndistortPoints( &_imagePoints1, &_imagePoints1,
//                       &_M1, &_D1, 0, &_M1 );
//    cvUndistortPoints( &_imagePoints2, &_imagePoints2,
//                       &_M2, &_D2, 0, &_M2 );
//    cvComputeCorrespondEpilines( &_imagePoints1, 1, &_F, &_L1 );
//    cvComputeCorrespondEpilines( &_imagePoints2, 2, &_F, &_L2 );
//    double avgErr = 0;
//    for( i = 0; i < N; i++ )
//    {
//        double err = fabs(points[0][i].x*lines[1][i].x +
//                          points[0][i].y*lines[1][i].y + lines[1][i].z)
//                     + fabs(points[1][i].x*lines[0][i].x +
//                            points[1][i].y*lines[0][i].y + lines[0][i].z);
//        avgErr += err;
//    }
//    printf( "avg err = %g\n", avgErr/(nframes*n) );
//
//    int height = imageSize.height;
//    int width  = imageSize.width;
//
//    // ���� ������ �����ϰ� ������ ����
//    if( showUndistorted )
//    {
//        CvMat* mx1   = cvCreateMat( height, width, CV_32F );
//        CvMat* my1   = cvCreateMat( height, width, CV_32F );
//        CvMat* mx2   = cvCreateMat( height, width, CV_32F );
//        CvMat* my2   = cvCreateMat( height, width, CV_32F );
//        CvMat* img1r = cvCreateMat( height, width, CV_8U  );
//        CvMat* img2r = cvCreateMat( height, width, CV_8U  );
//        CvMat* disp  = cvCreateMat( height, width, CV_16S );
//        CvMat* vdisp = cvCreateMat( height, width, CV_8U  );
//        CvMat* pair;
//        double R1[3][3], R2[3][3], P1[3][4], P2[3][4];
//        CvMat _R1 = cvMat(3, 3, CV_64F, R1);
//        CvMat _R2 = cvMat(3, 3, CV_64F, R2);
//
//        // BOUGUET ����
//        if( useUncalibrated == 0 )
//        {
//            CvMat _P1 = cvMat(3, 4, CV_64F, P1);
//            CvMat _P2 = cvMat(3, 4, CV_64F, P2);
//            cvStereoRectify( &_M1, &_M2, &_D1, &_D2, imageSize,
//                             &_R, &_T,
//                             &_R1, &_R2, &_P1, &_P2, 0,
//                             0 /*CV_CALIB_ZERO_DISPARITY*/ );
//            isVerticalStereo = fabs(P2[1][3]) > fabs(P2[0][3]);
//
//            // cvRemap() �Լ����� ������ map���� ����
//            cvInitUndistortRectifyMap(&_M1, &_D1, &_R1, &_P1, mx1, my1);
//            cvInitUndistortRectifyMap(&_M2, &_D2, &_R2, &_P2, mx2, my2);
//        }
//
//            // HARTLEY ����
//        else if( useUncalibrated == 1 || useUncalibrated == 2 )
//        {
//            double H1[3][3], H2[3][3], iM[3][3];
//            CvMat _H1 = cvMat(3, 3, CV_64F, H1);
//            CvMat _H2 = cvMat(3, 3, CV_64F, H2);
//            CvMat _iM = cvMat(3, 3, CV_64F, iM);
//
//            if( useUncalibrated == 2 )
//                cvFindFundamentalMat( &_imagePoints1,
//                                      &_imagePoints2, &_F);
//            cvStereoRectifyUncalibrated( &_imagePoints1,
//                                         &_imagePoints2, &_F,
//                                         imageSize,
//                                         &_H1, &_H2, 3);
//            cvInvert(&_M1, &_iM);
//            cvMatMul(&_H1, &_M1, &_R1);
//            cvMatMul(&_iM, &_R1, &_R1);
//            cvInvert(&_M2, &_iM);
//            cvMatMul(&_H2, &_M2, &_R2);
//            cvMatMul(&_iM, &_R2, &_R2);
//
//            // cvRemap() �Լ����� ������ map���� ����
//            cvInitUndistortRectifyMap(&_M1,&_D1,&_R1,&_M1,mx1,my1);
//            cvInitUndistortRectifyMap(&_M2,&_D1,&_R2,&_M2,mx2,my2);
//        }
//        else
//            assert(0);
//        cvNamedWindow( "rectified", 1 );
//
//        // ������ �����ϰ� ���� ������ ���Ѵ�.
//        if( !isVerticalStereo )
//            pair = cvCreateMat( height, width*2, CV_8UC3 );
//        else
//            pair = cvCreateMat( height*2, width, CV_8UC3 );
//
//        // ���׷��� ������ ���� ����
//        CvStereoBMState *BMState = cvCreateStereoBMState();
//        assert(BMState != 0);
//        BMState->preFilterSize=41;
//        BMState->preFilterCap=31;
//        BMState->SADWindowSize=41;
//        BMState->minDisparity=-64;
//        BMState->numberOfDisparities=128;
//        BMState->textureThreshold=10;
//        BMState->uniquenessRatio=15;
//
//        for( i = 0; i < nframes; i++ )
//        {
//            IplImage* img1 = cvLoadImage(imageNames[0][i].c_str(), 0);
//            IplImage* img2 = cvLoadImage(imageNames[1][i].c_str(), 0);
//            if( img1 && img2 )
//            {
//                CvMat part;
//                cvRemap( img1, img1r, mx1, my1 );
//                cvRemap( img2, img2r, mx2, my2 );
//                if( !isVerticalStereo || useUncalibrated != 0 )
//                {
//                    cvFindStereoCorrespondenceBM( img1r, img2r, disp,
//                                                  BMState);
//                    cvNormalize( disp, vdisp, 0, 256, CV_MINMAX );
//                    cvNamedWindow( "disparity" );
//                    cvShowImage( "disparity", vdisp );
//                }
//                if( !isVerticalStereo )
//                {
//                    cvGetCols( pair, &part, 0, width );
//                    cvCvtColor( img1r, &part, CV_GRAY2BGR );
//                    cvGetCols( pair, &part, width,
//                               width*2 );
//                    cvCvtColor( img2r, &part, CV_GRAY2BGR );
//                    for( j = 0; j < height; j += 16 )
//                        cvLine( pair, cvPoint(0,j),
//                                cvPoint(width*2,j),
//                                CV_RGB(0,255,0));
//                }
//                else
//                {
//                    cvGetRows( pair, &part, 0, height );
//                    cvCvtColor( img1r, &part, CV_GRAY2BGR );
//                    cvGetRows( pair, &part, height,
//                               height*2 );
//                    cvCvtColor( img2r, &part, CV_GRAY2BGR );
//                    for( j = 0; j < width; j += 16 )
//                        cvLine( pair, cvPoint(j,0),
//                                cvPoint(j,height*2),
//                                CV_RGB(0,255,0));
//                }
//                cvShowImage( "rectified", pair );
//                if( cvWaitKey() == 27 )
//                    break;
//            }
//            cvReleaseImage( &img1 );
//            cvReleaseImage( &img2 );
//        }
//        cvReleaseStereoBMState( &BMState );
//        cvReleaseMat( &mx1 );
//        cvReleaseMat( &my1 );
//        cvReleaseMat( &mx2 );
//        cvReleaseMat( &my2 );
//        cvReleaseMat( &img1r );
//        cvReleaseMat( &img2r );
//        cvReleaseMat( &disp  );
//        cvReleaseMat( &vdisp );
//        cvReleaseMat( &pair );
//    }
//
//    cvDestroyAllWindows();
}

int main(void)
{
    StereoCalib("/home/m/Desktop/PROGRAMMING/Clion_code/list.txt", 9, 6, 0);
    return 0;
}
