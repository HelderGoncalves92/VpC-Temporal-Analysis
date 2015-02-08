//
//  main.cpp
//  VC-Projeto
//
//  Created by Hélder Gonçalves on 11/12/14.
//  Copyright (c) 2014 Hélder Gonçalves. All rights reserved.
//

#include "includes.hpp"

typedef struct sPos{
    int i;
    int j;
    struct sPos *next;
}*Pos,NPos;


Pos addPos(Pos pos, int i, int j){
    Pos p=(Pos)malloc(sizeof(NPos));
    p->i=i;
    p->j=j;
    p->next=pos;
    
    return p;
}




// rearrange the quadrants of Fourier image  so that the origin is at the image center
void fftShift(Mat image){
    int cy,cx;
    
    cx = image.cols/2;
    cy = image.rows/2;
    
    Mat q0(image, Rect(0, 0, cx, cy));   // Top-Left - Create a ROI per quadrant
    Mat q1(image, Rect(cx, 0, cx, cy));  // Top-Right
    Mat q2(image, Rect(0, cy, cx, cy));  // Bottom-Left
    Mat q3(image, Rect(cx, cy, cx, cy)); // Bottom-Right
    
    Mat tmp;                           // swap quadrants (Top-Left with Bottom-Right)
    q0.copyTo(tmp);
    q3.copyTo(q0);
    tmp.copyTo(q3);
    
    q1.copyTo(tmp);                    // swap quadrant (Top-Right with Bottom-Left)
    q2.copyTo(q1);
    tmp.copyTo(q2);
    
    normalize(image, image, 0, 1, CV_MINMAX); // Transform the matrix with float values into a
    // viewable image form (float between values 0 and 1).
    
}


void findRho_impf(Mat rho, Mat magI, int *num, float *f){
    int i,j, r, min=1000, max=0;

    for (i=0; i<rho.cols; i++) {
        for (j=0; j<rho.rows; j++) {
            r=(int)rho.at<float>(j, i);
            

            if(r>0 &&  r <= rho.cols/2+1){
				f[r] += powf(magI.at<float>(j, i), 2);
				num[r]++;
            }
        }
    }
	for (r = 1; r<magI.cols / 2; r++)
		f[r] /= num[r];
}


int main(){
    
    int m, n, r, i, j, mX, mY ;
    Mat frame, padded;
    Mat planes[2], complexI;
    Mat magI;
    
    //VideoCapture cap(0); // open the default camera
	VideoCapture cap("Videos/approaching_lv_40ms_translate_approach.avi"); // open the default camera
    if(!cap.isOpened())  // check if we succeeded
        return -1;
    
    Mat edges;
    namedWindow("edges",1);
    
    //Inicializar a estrutura
    int *num = (int*)calloc(sizeof(int),n/2+2);
    float *f=(float*)calloc(sizeof(float),n/2+2);

    cap >> frame; // get a new frame from camera

	m = frame.rows;
	n = frame.cols;

    int mdst = getOptimalDFTSize( m );

	/******* Get coordinates of the power spectrum image *******/
	Mat xx = Mat::zeros(Size(mdst, mdst), CV_32F);
	Mat yy = Mat::zeros(Size(mdst, mdst), CV_32F);
	Mat theta, rho;

	mY = -mdst / 2;
	for (i = 0; i<mdst; i++) {
		mX = -mdst / 2;
		for (j = 0; j<mdst; j++) {
			xx.at<float>(i, j) = (float)mX++;
			yy.at<float>(i, j) = (float)mY;
		}
		// printf("%d ",mY);
		mY++;
	}
	cartToPolar(xx, yy, rho, theta);


	//Round
	for (i = 0; i<mdst; i++) {
		for (j = 0; j<mdst; j++) {
			rho.at<float>(j, i) = cvRound(rho.at<float>(j, i));
		}
	}


	int nframe = 0;
	while (cap.isOpened()) {

		nframe++;
        
		if (nframe != 1)
			cap >> frame; // get a new frame from camera

        cvtColor(frame, edges, CV_BGR2GRAY);
        
        
        //printf("1-%d %d\n",m,n);
        // on the border add zero values
		copyMakeBorder(edges, padded, 0, mdst - edges.rows, 0, mdst - edges.cols, BORDER_CONSTANT, Scalar::all(0));
        
        // Add to the expanded another plane with zeros
        planes[0] = Mat_<float>(padded);
        planes[1] = Mat::zeros(padded.size(), CV_32F);
        merge(planes, 2, complexI);
        
        dft(complexI, complexI);            // this way the result may fit in the source matrix
        
        /************************  MAGNITUDE *******************************/
        // compute the magnitude and switch to logarithmic scale
        // => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
        split(complexI, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
        magnitude(planes[0], planes[1], planes[0]);// planes[0] = magnitude
        magI = planes[0];
        
        /************************  Expand and Rearrange *******************************/
        // switch to logarithmic scale
        magI += Scalar::all(1);
        log(magI, magI);
        
        
        // crop the spectrum, if it has an odd number of rows or columns
        magI = magI(Rect(0, 0, magI.cols & -2, magI.rows & -2));
        
        
        /************************  fftShift  *******************************/
        // rearrange the quadrants of Fourier image  so that the origin is at the image center
        fftShift(magI);
        
        
        

        /************************** CICLO *******************************/
        
        
		findRho_impf(rho, magI, num,f);
		


		Vec4f line;
		vector<Point2f> points;
		int i = 0, tam = magI.rows/2+1;
		for (i=2; i < tam;i++)
				points.push_back(Point2f((float)log(i), (float)log(f[i])));

		fitLine(points, line, CV_DIST_L1, 0, 0.1, 0.11);
		float alpha = 0.0f;
		//if(line[0] != 0.0f)
			alpha = line[1] / line[0];
		printf("%f\n", alpha);
		//printf("%f - %f %f %f %f\n", alpha, line[0], line[1], line[2], line[3]);


		

       // for (j=1; j<magI.cols/2+1; j++)
         //   printf("%f ", f[j]);
        
        //print
        /* for (i=0; i<magI.cols; i++) {
         for (j=0; j<magI.rows; j++) {
         printf("%d ",(int)rho.at<float>(j, i));
         }
         }*/

        
        /*
        m=6;
        Mat xx = Mat::zeros(m , m, CV_32F);
        Mat yy = Mat::zeros(m , m, CV_32F);
        Mat theta, rho;
        
        mY=-m/2;
        for (i=0; i<m; i++) {
            mX=-m/2;
            for (j=0; j<m; j++) {
                xx.at<float>(i, j)=(float)mX++;
                yy.at<float>(i, j)=(float)mY;
            }
            // printf("%d ",mY);
            mY++;
        }
        
        
        
        
        cartToPolar(xx, yy, rho, theta);
        
        for (i=0; i<xx.rows; i++) {
            for (j=0; j<xx.cols; j++) {
                printf("%f ",(float)rho.at<float>(i, j));
            }
            printf("\n");
        }
        */
        
        

        
        
        
        
        
        
        imshow("spectrum magnitude", magI);
        if(waitKey(30) >= 0) break;
		memset(f, 0, (n / 2 + 2) * sizeof(float));


    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    
	cap.release();
    return 1;
}
