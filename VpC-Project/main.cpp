//
//  main.cpp
//  VC-Projeto
//
//  Created by Hélder Gonçalves on 11/12/14.
//  Copyright (c) 2014 Hélder Gonçalves. All rights reserved.
//

#include "includes.hpp"

int arraySize;



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
    int i,j, r;

    for (i=0; i<rho.cols; i++) {
        for (j=0; j<rho.rows; j++) {
            r=(int)rho.at<float>(j, i);
            
            if(r>0 &&  r <= arraySize){
				f[r] += powf(magI.at<float>(j, i), 2);
				num[r]++;
            }
        }
    }
    for (r = 1; r<arraySize; r++)
        if(num[r]!=0)
            f[r] /= num[r];
}


int main(){
    
    int m, i, j, mX, mY ;
    Mat frame, padded;
    Mat planes[2], complexI;
    Mat magI;
    
    Vec4f line;
    vector<Point2f> points;
    
    //VideoCapture cap(0); // open the default camera
	VideoCapture cap("videos/approaching_lv_40ms_translate_approach.avi"); // open the default camera
    if(!cap.isOpened())  // check if we succeeded
        return -1;
    
    Mat edges;
    namedWindow("edges",1);

    cap >> frame; // get a new frame from camera

    //Image is square, just need know one side
    m = frame.rows;

    int mdft = getOptimalDFTSize( m );
    arraySize = mdft/2+1;
    
    //Init arrays
    int *num = (int*)calloc(sizeof(int),arraySize);
    float *f=(float*)calloc(sizeof(float),arraySize);
    
    

	/************* Get coordinates of the power spectrum image ***********/
	Mat xx = Mat::zeros(Size(mdft, mdft), CV_32F);
	Mat yy = Mat::zeros(Size(mdft, mdft), CV_32F);
	Mat theta, rho;

	mY = -mdft / 2;
	for (i = 0; i<mdft; i++) {
		mX = -mdft / 2;
		for (j = 0; j<mdft; j++) {
			xx.at<float>(i, j) = (float)mX++;
			yy.at<float>(i, j) = (float)mY;
		}
		mY++;
	}
	cartToPolar(xx, yy, rho, theta);


	//Round RHO
	for (i = 0; i<mdft; i++)
		for (j = 0; j<mdft; j++)
			rho.at<float>(j, i) = cvRound(rho.at<float>(j, i));


	int nframe = 0;
    
    /************** START CYCLE ******************/
	while (cap.isOpened()) {

        cap >> frame; // get a new frame from camera
        if(!frame.data) break;

        nframe++;
        cvtColor(frame, edges, CV_BGR2GRAY);
        
        // on the border add zero values
		copyMakeBorder(edges, padded, 0, mdft - edges.rows, 0, mdft - edges.cols, BORDER_CONSTANT, Scalar::all(0));
        
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
        
        

        /************************OBTAIN ARRAY 'F' TO DIFERENTS 'R' *******************************/
		findRho_impf(rho, magI, num,f);
		
        
        
		for (i=2; i < arraySize;i++)
            points.push_back(Point2f((float)log(i), (float)log(f[i])));

		fitLine(points, line, CV_DIST_L1, 0, 0.1, 0.11);
		float alpha = 0.0f;
        
		
        alpha = line[1] / line[0];
        add_alpha(alpha);
        
        
        //Reset arrays 'F' & 'points' to a new iteration
		memset(f, 0, arraySize * sizeof(float));
        points.clear();
        
        
        imshow("spectrum magnitude", frame);
        if(waitKey(30) >= 0) break;
        
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    
    cout << nframe;
	cap.release();
    return 1;
}
