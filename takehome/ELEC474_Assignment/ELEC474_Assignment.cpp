// ELEC474_Assignment.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp> // OpenCV Core Functionality
#include <opencv2/highgui/highgui.hpp> // High-Level Graphical User Interface
#include "opencv2/features2d.hpp" 


using namespace std;
using namespace cv;

#define STEP1 1
#define STEP2 1
#define STEP3 1

//global setting flags 
//rescales the images so things arnt slow as shit, set to 1 for no rescaling
#define RESCALE_ON_LOAD 0.5
#define UNDISTORT_ON_LOAD 1
#define SMART_ADD_GAUSIAN_BLUR 101
//JSUT BE SMART WITH THESE probly best ot leave at 2 for now
#define PADDING_AMMOUNT 2
#define PADDING_OFFSET 2

//debug flags, toggles certain image diplay and logging 
#define IMAGE_LOADING_DEBUG 1  //shows loaded images
#define IMAGE_SMART_ADD_DEBUG 1 //shows masks and stuff
#define IMAGE_MATCHING_DEBUG 1 //shows matched points, tranfomation matrixes, warped images, etc

//for debugging, so we dont need to load the whole folder
#define MAX_IMAGES_TO_LOAD 3


vector<string> goodImages;

//preps the global image info, this is done mostly with speclation 

//
//Mat translateImg(Mat& img, int offsetx, int offsety) {
//	Mat trans_mat = (Mat_<double>(2, 3) << 1, 0, offsetx, 0, 1, offsety);
//	warpAffine(img, img, trans_mat, img.size());
//	return img;
//}


//Class for storing an image, the img mat, name, features etc go in a lsit of this type
class ourImage {
public:
	string path;
	string name;
	Mat img;
	Mat imgGrey;

	//CONTRUCTOR
	ourImage( string path) {
		this->path = path;
		//load the image, but center it with a black border half its size
		Mat distorted = imread(path);
		Mat temp = Mat(distorted.rows, distorted.cols, distorted.type());
		//rescale if specified
		if (RESCALE_ON_LOAD != 1) {
			resize(distorted, distorted, Size(), RESCALE_ON_LOAD, RESCALE_ON_LOAD);
		}


		//undistort it if option set
		if (UNDISTORT_ON_LOAD) {
			Mat intrinsic = (Mat_<double>(3, 3) << 600, 0, distorted.cols / 2, 0, 600, distorted.rows / 2, 0, 0, 1);//camera matrix
			Mat distortionCoef = (Mat_<double>(1, 5) << 0.2, 0.05, 0.00, 0, 0); //radial blur coefs
			Mat camMatrix = getOptimalNewCameraMatrix(intrinsic, distortionCoef, distorted.size(), 0);//make the actual transforma matrix 
			if (IMAGE_LOADING_DEBUG) {
				cout << "Cam matrix" << endl;
				cout << camMatrix << endl;
			}
			temp = Mat(distorted.rows, distorted.cols, distorted.type());
			undistort(distorted, temp, camMatrix, distortionCoef);
		}
		else {
			temp = distorted;
		}
		
		// center it with a black border PADDING_AMMOUNT its size
		this->img = Mat(temp.rows * PADDING_AMMOUNT, temp.cols * PADDING_AMMOUNT, temp.type());
		Mat trans_mat = (Mat_<double>(2, 3) << 1, 0, temp.cols / PADDING_OFFSET, 0, 1, temp.rows / PADDING_OFFSET);
		warpAffine(temp, this->img, trans_mat, this->img.size());
		
		cvtColor(this->img, this->imgGrey, COLOR_RGB2GRAY);
	}
};
//set of the images 
vector<ourImage> imageSet;

//takes two images and adds them, img2 on top with some edge bluring 
Mat smartAddImg(Mat & img_1, Mat & img_2) {
	//solid mask
	Mat solidMask = Mat::zeros(img_2.rows, img_2.cols, CV_8U);
	for (int r = 0; r < img_2.rows; r++) {
		for (int c = 0; c < img_2.cols; c++) {
			if (img_2.at<Vec3b>(r, c) != Vec3b(0, 0, 0)) {
				solidMask.at<unsigned char>(r, c) = 255;
			}
		}
	}
	//erode it a bit (gets rid of fine black outline)
	int erosion_size = 10;
	Mat element = getStructuringElement(MORPH_RECT,
		Size(2 * erosion_size + 1, 2 * erosion_size + 1),
		Point(erosion_size, erosion_size));
	erode(solidMask, solidMask, element);

	//diplay if debug flag
	if (IMAGE_SMART_ADD_DEBUG) {
		namedWindow("solidMask", WINDOW_NORMAL);
		imshow("solidMask", solidMask);
		resizeWindow("solidMask", 600, 600);
	}
	//Blur the mask for use in blending, wraped this in a try cause blurs can be finicky  
	Mat bluredMask = Mat(img_2.rows, img_2.cols, CV_8U);
	try {
		GaussianBlur(solidMask, bluredMask, Size(SMART_ADD_GAUSIAN_BLUR, SMART_ADD_GAUSIAN_BLUR), 0, 0);
		if (IMAGE_SMART_ADD_DEBUG) {
			namedWindow("bluredMask", WINDOW_NORMAL);
			imshow("bluredMask", bluredMask);
			resizeWindow("bluredMask", 600, 600);
		}
	}
	catch (const std::exception & e) {
		cout << "Blur error, somethign is wrong" << endl;
		cout << e.what() << endl;
		//bluredMask = solidMask;
	}
	//actually compute the new image on top of image 1 
	for (int r = 0; r < img_2.rows; r++) {
		for (int c = 0; c < img_2.cols; c++) {
			if (solidMask.at<unsigned char>(r, c) == 255) {
				Vec3b img2Pixel = img_2.at<Vec3b>(r, c);
				Vec3b img1Pixel = img_1.at<Vec3b>(r, c);
				if (img1Pixel != Vec3b(0, 0, 0)) {
					unsigned char mix = bluredMask.at<unsigned char>(r, c);
					img_1.at<Vec3b>(r, c) = (img1Pixel * double(double(255 - mix) / double(255))) + (img2Pixel * double(double(mix) / double(255)));
				}
				else {
					img_1.at<Vec3b>(r, c) = img2Pixel;
				}
			}
			
		}
	}
	

	return img_1 ;
}


//Composite 2 images by feature masking 
Mat composite2Images(Mat& img_1, Mat& img_2) {
	//intitate orb detector 
	vector<KeyPoint> keypoints_1, keypoints_2;
	Mat descriptors_1, descriptors_2;
	Ptr<FeatureDetector> detector = ORB::create();
	Ptr<DescriptorExtractor> descriptor = ORB::create();
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");

	//detect points and compute descriptors
	detector->detect(img_1, keypoints_1);
	detector->detect(img_2, keypoints_2);
	descriptor->compute(img_1, keypoints_1, descriptors_1);
	descriptor->compute(img_2, keypoints_2, descriptors_2);

	//draw keypoints
	Mat outimg1;
	drawKeypoints(img_1, keypoints_1, outimg1, Scalar::all(-1), DrawMatchesFlags::DEFAULT);

	vector<DMatch> matches;
	//BFMatcher matcher ( NORM_HAMMING );
	matcher->match(descriptors_1, descriptors_2, matches);
	double min_dist = 10000, max_dist = 0;

	//Find the minimum and maximum distances between mathces, so the most similar and the least similar 
	for (int i = 0; i < descriptors_1.rows; i++)
	{
		double dist = matches[i].distance;
		if (dist < min_dist) min_dist = dist;
		if (dist > max_dist) max_dist = dist;
	}

	printf("-- Max dist : %f \n", max_dist);
	printf("-- Min dist : %f \n", min_dist);

	std::vector< DMatch > good_matches;
	//When the distance between the descriptors is 
	//greater than twice the minimum distance, the match is considered to be incorrect. 
	//But sometimes the minimum distance will be very small, and an empirical value of 30 is set as the lower limit.
	for (int i = 0; i < descriptors_1.rows; i++)
	{
		if (matches[i].distance <= max(2 * min_dist, 30.0))
		{
			good_matches.push_back(matches[i]);
		}
	}
	Mat img_match;
	Mat img_goodmatch;
	//-- Draw results 
	if (IMAGE_MATCHING_DEBUG) {
		
		drawMatches(img_1, keypoints_1, img_2, keypoints_2, matches, img_match);
		drawMatches(img_1, keypoints_1, img_2, keypoints_2, good_matches, img_goodmatch);
		namedWindow("matches", WINDOW_NORMAL);
		imshow("matches", img_match);
		resizeWindow("matches", 600, 600);
		namedWindow("goodmatches", WINDOW_NORMAL);
		imshow("goodmatches", img_goodmatch);
		resizeWindow("goodmatches", 800, 800);
	}

	//estimate affine transfomation 
	//get points to use 
	if (IMAGE_MATCHING_DEBUG) {
		cout << "points used to calc transform" << endl;
	}
	vector<Point2d> transformPtsImg1;
	vector<Point2d> transformPtsImg2;
	//get the good points, take top 30
	for (int i = 0; (i < good_matches.size() && i < 30); i++) {
		transformPtsImg1.push_back(keypoints_1[good_matches[i].queryIdx].pt);
		transformPtsImg2.push_back(keypoints_2[good_matches[i].trainIdx].pt);
		if (IMAGE_MATCHING_DEBUG) {
			cout << transformPtsImg1[i] << " -> " << transformPtsImg2[i] << endl;
		}

	}
	//calcualte transfomation matrix
	Mat homo = findHomography(transformPtsImg2, transformPtsImg1, RANSAC, 5.0);
	if (IMAGE_MATCHING_DEBUG) {
		cout << "Transfomation Matrix" << endl;
		cout << homo << endl;
	}

	//apply transformation to image 
	Mat warpedImg = Mat(img_2.rows, img_2.cols, img_2.type());
	warpPerspective(img_2, warpedImg, homo, warpedImg.size());

	if (IMAGE_MATCHING_DEBUG) {
		namedWindow("warpedIMG", WINDOW_NORMAL);
		imshow("warpedIMG", warpedImg);
		resizeWindow("warpedIMG", 800, 800);
	}

	//compose images
	Mat compositeImg;

	compositeImg = smartAddImg(img_1, warpedImg);
	//addWeighted(img_1, 0.5, warpedImg, 0.5, 1, compositeImg);
	//display
	namedWindow("compositeIMG", WINDOW_NORMAL);
	imshow("compositeIMG", compositeImg);
	resizeWindow("compositeIMG", 800, 800);

	return compositeImg;
}




int main(int argc, char* argv[])
{

	cout << "current program running from direcotry" << endl;
	cout << filesystem::current_path() << endl;

	string folderPath = "office2";
	//string folderPath = "WLH";
	//string folderPath = "StJames";
	cout << "opening " << MAX_IMAGES_TO_LOAD << " images from " << folderPath << " folder" << endl;
	try {
		int imagesLoaded = 0;
		for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
			if (imagesLoaded < MAX_IMAGES_TO_LOAD){
				cout << entry.path() << std::endl;
				imageSet.push_back(entry.path().string());//c++ is so weird, i just need to pass in the construtor params and not a new instance 
				imagesLoaded++;
			}
		}
	}
	catch (const std::exception & e) { 
		//probably couldnt find the folder
		cout << e.what() << endl;
	}
	if (IMAGE_LOADING_DEBUG) {
		for (ourImage img : imageSet) {
			//show the images
			namedWindow(img.path, WINDOW_NORMAL);
			imshow(img.path, img.img);
			resizeWindow(img.path, 600, 600);
		}
	}
	
	//waitKey();
	if (STEP2) {
		Mat tempComp = composite2Images(imageSet[0].img, imageSet[1].img);
		composite2Images(tempComp, imageSet.back().img);
	}
	waitKey(0);






    //std::cout << "Hello World!\n";
	/*if (STEP3) {
		generateCompositeImage(goodImages);
	}*/
}

//void generateCompositeImage(vector<string> good) {
//	return;
//}