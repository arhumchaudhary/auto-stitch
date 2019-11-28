/*	ELEC474 Take Home Exam
		Auto-Stitch
	Brent Champion | 20066282
	Nathaniel Pauze | 20066234
	
	COMMITS
	1.	Started File solution (and didnt do it right apparently) - Brent
	2.	Implements functinos to basically do whole program - Nat
	3.	Implements some clean up, modifies main to allow easier toggling between
		how many images to load and from which folder. Needs some work on the stitching
		but overall it is producing an output th at looks good - Brent

	Comments:
	-	Need to add radiometric transformation things

	-	OK so the software is basically picking an image and 
		having that remain the background like 'flat' plane for
		the entire stitching program. I think we should instead try to pick
		the middle image and stitch to the left and stitch to the right instead
		This will make it look more like a real stitched and will make it less 
		like were crowding the right side. Thoughts?

	-	Clean up the routines so that we're basically calling step1(), step2() 
		and step3() in the main loop that iterates over all of the images in 
		a folder. This way it is clear where the functions are relative to the 
		instructions they gave us are. Thoughts?

	-	Add toggle for matching images because every iteration of the pipeline 
		overwrites the last one, would be better if it showed them all.

*/

#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp> // OpenCV Core Functionality
#include <opencv2/highgui/highgui.hpp> // High-Level Graphical User Interface
#include "opencv2/features2d.hpp" 
//#include "opencv2/nonfree.hpp"


using namespace std;
using namespace cv;

/* Global Settings */

#define STEP1 1 //load images
#define STEP2 1	//compute matches bwtween all
#define STEP3 0 //find optimal "path" to stich all  images 
#define STEP4 0 //perform stiching

#define RESCALE_ON_LOAD 0.5 // Rescaling the images for faster program
#define UNDISTORT_ON_LOAD 1
#define SMART_ADD_GAUSIAN_BLUR 101

#define PIXEL_PADDING 600 //how many pixels should pad each image 

#define PADDING_AMMOUNT 2
#define PADDING_OFFSET 2

// Debug Flags
#define IMAGE_LOADING_DEBUG 1  // Show loaded image (original)
#define IMAGE_SMART_ADD_DEBUG 0 // Shows the masks of images 
#define IMAGE_MATCHING_DEBUG 0 // Shows matched points, tranfomation matrixes, etc - nice
#define IMAGE_COMPOSITE_DEBUG 1
// Avoiding loading the whole folder

#define MAX_IMAGES_TO_LOAD 10 

/* Global Variables */

/* Folder Path Settings
1 - Office
2 - WLH
3 - StJames
*/

#define FOLDER 1
string folderPath; // Global for folder path




//string folderPath = "WLH";
//string folderPath = "StJames";

vector<string> goodImages; // Vector to hold the path of the good images

Mat PadImage(Mat& img);

Mat translateImg(Mat& img, Mat& target, int offsetx, int offsety) {
	Mat trans_mat = (Mat_<double>(2, 3) << 1, 0, offsetx, 0, 1, offsety);
	warpAffine(img, target, trans_mat, img.size());
	return img;
}

void setFolderPath() {
	if (FOLDER == 1) {
		folderPath = "office2";
	}
	else if (FOLDER == 2) {
		folderPath = "WLH";
	}
	else if (FOLDER == 3) {
		folderPath = "StJames";
	}
	else { cout << "Invalid FOLDER choice"; return; }
}

//Class for storing an image, the img mat, name, features etc go in a lsit of this type
class ourImage {
public:
	string path; // File path
	string name; // File name
	Mat img;
	Mat imgGrey;
	vector<vector<DMatch>> goodMatches;
	vector<double> goodMatchScores;
	vector<Mat> homographyMatrixes;
	//CONTRUCTOR
	ourImage(string path) {
		goodMatches.resize(MAX_IMAGES_TO_LOAD);
		goodMatchScores.resize(MAX_IMAGES_TO_LOAD);
		homographyMatrixes.resize(MAX_IMAGES_TO_LOAD);

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
		
		this->img = PadImage(temp);
		//// center it with a black border PADDING_AMMOUNT its size
		//this->img = Mat(temp.rows * PADDING_AMMOUNT, temp.cols * PADDING_AMMOUNT, temp.type());
		//Mat trans_mat = (Mat_<double>(2, 3) << 1, 0, temp.cols / PADDING_OFFSET, 0, 1, temp.rows / PADDING_OFFSET);
		//warpAffine(temp, this->img, trans_mat, this->img.size());
		
		cvtColor(this->img, this->imgGrey, COLOR_RGB2GRAY);
	}
};

//set of the images
vector<ourImage> imageSet;
//indexes of the images in our composite allready
vector<int> imagesInComposite;
vector<int[2]> pathToassemble;

//function 
Mat PadImage(Mat&  img) {
	int currentPaddingTop = 0;
	int currentPaddingLeft = 0;
	int currentPaddingBottom = 0;
	int currentPaddingRight = 0;

	//so aparently the only good way to break out of nested loops in c++ is goto, go figure


	//start from top 
	for (int r = 0; r < img.rows; r++) {
		for (int c = 0; c < img.cols; c++) {
			if (img.at<Vec3b>(r, c) != Vec3b(0, 0, 0)) {
				currentPaddingTop = r;
				goto leftCheck;
			}
		}
	}
	leftCheck:
	//start from left 
	for (int c = 0; c < img.cols; c++) {
		for (int r = 0; r < img.rows; r++) {
			if (img.at<Vec3b>(r, c) != Vec3b(0, 0, 0)) {
				currentPaddingLeft = c;
				goto bottomCheck;
			}
		}
	}
	bottomCheck:
	//start from bottom
	for (int r = img.rows-1; r >=0; r--) {
		for (int c = 0; c < img.cols; c++) {
			if (img.at<Vec3b>(r, c) != Vec3b(0, 0, 0)) {
				currentPaddingBottom = img.rows - r -1;
				goto rightCheck;
			}
		}
	}
	rightCheck:
	//start from right
	for (int c = img.cols - 1; c >= 0; c--) {
		for (int r = 0; r < img.rows; r++) {
			if (img.at<Vec3b>(r, c) != Vec3b(0, 0, 0)) {
				currentPaddingRight = img.cols -c -1;
				goto endOfChecks;
			}
		}
	}
	endOfChecks:


	cout << "Image is " << img.rows << " rows by " << img.cols << " cols" << endl;
	cout << "Top padding: " << currentPaddingTop << " Left padding: " << currentPaddingLeft << " Bottom padding: " << currentPaddingBottom << " Right padding: " << currentPaddingRight << endl;
	

	int paddingNeededTop = 0;
	int paddingNeededLeft = 0;
	int paddingNeededBottom = 0;
	int paddingNeededRight = 0;
	if (currentPaddingTop < PIXEL_PADDING) {
		paddingNeededTop = PIXEL_PADDING - currentPaddingTop;
	}
	if (currentPaddingLeft < PIXEL_PADDING) {
		paddingNeededLeft = PIXEL_PADDING - currentPaddingLeft;
	}
	if (currentPaddingBottom < PIXEL_PADDING) {
		paddingNeededBottom = PIXEL_PADDING - currentPaddingBottom;
	}
	if (currentPaddingRight < PIXEL_PADDING) {
		paddingNeededRight = PIXEL_PADDING - currentPaddingRight;
	}
	int offsetx = paddingNeededLeft;
	int offsety = paddingNeededTop;

	Mat newImage = Mat::zeros(img.rows + paddingNeededTop + paddingNeededBottom, img.cols + paddingNeededLeft + paddingNeededRight, CV_8U);

	Mat trans_mat = (Mat_<double>(2, 3) << 1, 0, offsetx, 0, 1, offsety);
	warpAffine(img, newImage, trans_mat, newImage.size());
	cout << "new image dimensions " << newImage.rows << " rows by " << newImage.cols << " cols" << endl;
	return newImage;

}


//takes two images and adds them, img2 on top with some edge bluring 
Mat smartAddImg(Mat & img_1, Mat & img_2) {
	//solid mask
	Mat solidMask = Mat::zeros(img_2.rows, img_2.cols, CV_8U);
	Mat erodedMask = Mat::zeros(img_2.rows, img_2.cols, CV_8U);
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
	//erode again so blur is smother transition, ei edges of blur dont start at grey and not black
	erosion_size = SMART_ADD_GAUSIAN_BLUR;
	element = getStructuringElement(MORPH_RECT,
		Size(2 * erosion_size + 1, 2 * erosion_size + 1),
		Point(erosion_size, erosion_size));
	erode(solidMask, erodedMask, element);


	//diplay if debug flag
	if (IMAGE_SMART_ADD_DEBUG) {
		namedWindow("solidMask", WINDOW_NORMAL);
		imshow("solidMask", solidMask);
		resizeWindow("solidMask", 600, 600);
	}
	//Blur the mask for use in blending, wraped this in a try cause blurs can be finicky  
	Mat bluredMask = Mat(img_2.rows, img_2.cols, CV_8U);
	try {
		GaussianBlur(erodedMask, bluredMask, Size(SMART_ADD_GAUSIAN_BLUR, SMART_ADD_GAUSIAN_BLUR), 0, 0);
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

//compute best mathces between 2 images using the indexes in image set, find transform too
void FindMatches(int img1indx, int img2indx) {
	Mat& img_1 = imageSet[img1indx].img;
	Mat& img_2 = imageSet[img2indx].img;
	const int numOfPointsToGet =1000; //interestingly this has a minor effect on processign time.... weird
	double matchScore = 0;
	//intitate orb detector 
	vector<KeyPoint> keypoints_1, keypoints_2;
	Mat descriptors_1, descriptors_2;
	//Ptr<SIFT> detector = cv::xfeatures2d::SIFT::create;
	//Ptr<FeatureDetector> detector = ORB::create();
	Ptr<FeatureDetector> detector = ORB::create(numOfPointsToGet, 1.2, 8, 127, 0, 2, ORB::HARRIS_SCORE, 127, 20);
	Ptr<DescriptorExtractor> descriptor = ORB::create();
	Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");

	//detect points and compute descriptors
	detector->detect(img_1, keypoints_1);
	detector->detect(img_2, keypoints_2);
	descriptor->compute(img_1, keypoints_1, descriptors_1);
	descriptor->compute(img_2, keypoints_2, descriptors_2);

	//draw keypoints
	//Mat outimg1;
	//drawKeypoints(img_1, keypoints_1, outimg1, Scalar::all(-1), DrawMatchesFlags::DEFAULT);

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
		matchScore += (1+dist)*(1+dist);
	}
	//match score is the average of all the distances
	matchScore = double(double(matchScore) / double(numOfPointsToGet));


	printf("-Matches between img %d and %d -\n", img1indx, img2indx);
	//printf("-- Max dist : %f \n", max_dist);
	//printf("-- Min dist : %f \n", min_dist);
	printf("-- Match score : %f \n", matchScore);

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
	
	Mat img_goodmatch;
	//-- Draw results 
	if (IMAGE_MATCHING_DEBUG) {

		//drawMatches(img_1, keypoints_1, img_2, keypoints_2, matches, img_match);
		drawMatches(img_1, keypoints_1, img_2, keypoints_2, good_matches, img_goodmatch);
		//namedWindow("matches", WINDOW_NORMAL);
		//imshow("matches", img_match);
		//resizeWindow("matches", 600, 600);
		string window = "good matches between " + to_string(img1indx) + " and " + to_string(img2indx);
		namedWindow(window, WINDOW_NORMAL);
		imshow(window, img_goodmatch);
		resizeWindow(window, 800, 800);
	}
	//estimate affine transfomation 
	//get points to use 
	if (IMAGE_MATCHING_DEBUG) {
		cout << "points used to calc transform" << endl;
	}
	vector<Point2d> transformPtsImg1;
	vector<Point2d> transformPtsImg2;
	//get the good points, take top 30
	for (int i = 0; (i < good_matches.size() && i < 60); i++) {
		transformPtsImg1.push_back(keypoints_1[good_matches[i].queryIdx].pt);
		transformPtsImg2.push_back(keypoints_2[good_matches[i].trainIdx].pt);
		if (IMAGE_MATCHING_DEBUG) {
			//cout << transformPtsImg1[i] << " -> " << transformPtsImg2[i] << endl;
		}
	}
	//put the good points and the score of all points in the image set data
	imageSet[img1indx].goodMatches[img2indx] = good_matches;
	imageSet[img1indx].goodMatchScores[img2indx] = matchScore;
	imageSet[img2indx].goodMatches[img1indx] = good_matches;
	imageSet[img2indx].goodMatchScores[img1indx] = matchScore;
	
	//find the transform
	Mat homo1 = findHomography(transformPtsImg2, transformPtsImg1, RANSAC, 5.0);
	if (IMAGE_MATCHING_DEBUG) {
		cout << "Transfomation Matrix" << endl;
		cout << homo1 << endl;
	}
	Mat homo2 = findHomography(transformPtsImg1, transformPtsImg2, RANSAC, 5.0);
	if (IMAGE_MATCHING_DEBUG) {
		cout << "Transfomation Matrix" << endl;
		cout << homo2 << endl;
	}
	//put in the dataset for later
	imageSet[img1indx].homographyMatrixes[img2indx] = homo1;
	imageSet[img2indx].homographyMatrixes[img1indx] = homo2;
	


}




// Composite 2 images by feature masking 
Mat composite2Images(Mat& composite, int img1indx, int img2indx) {
	//Mat& img_1 = imageSet[img1indx].img;
	Mat& img_1 = composite;
	Mat& img_2 = imageSet[img2indx].img;
	Mat homo = imageSet[img1indx].homographyMatrixes[img2indx];

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
	string window = "composite Img using transform " + to_string(img1indx) + " and " + to_string(img2indx);
	namedWindow(window, WINDOW_NORMAL);
	imshow(window, compositeImg);
	resizeWindow(window, 800, 800);

	return compositeImg;
}

int main(int argc, char* argv[]){

	cout << "current program running from direcotry" << endl;
	cout << filesystem::current_path() << endl;

	setFolderPath();
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
	//claculate all matchign points and transfomrs and store that 
	if (STEP2) {
		for (int i = 0; i < MAX_IMAGES_TO_LOAD ; i++) {
			for (int j = 0; j < MAX_IMAGES_TO_LOAD ; j++) {
				if (j != i) {
					//if this is 0 we havent checked this pair yet
					if (imageSet[i].goodMatchScores[j] == 0) {
						FindMatches(i, j);
					}
				}
			}
		}
	}
	if (STEP3) {
		//pathToassemble.push_back({ 1,0 });
		//pathToassemble.push_back({ 1,2 });
	}




	if (STEP4) {
		Mat comp = imageSet[1].img;
		imagesInComposite.push_back(1);
		comp = composite2Images(comp,1, 0);
		imagesInComposite.push_back(0);
		comp = composite2Images(comp, 1, 2);
		imagesInComposite.push_back(2);
		//comp = composite2Images(comp, 0, 3);

		string window = "Final Composite Image of ";
		for (int i = 0; i < imagesInComposite.size(); i++) {
			window += imagesInComposite[i] + ",";
		}
		namedWindow(window, WINDOW_NORMAL);
		resizeWindow(window, 600, 600);
		imshow(window, comp);
	}

	////waitKey();
	//if (STEP3) {
	//	Mat& img_1;
	//	Mat middleman = imageSet[0].img;
	//	for (int i = 0; i < MAX_IMAGES_TO_LOAD - 1; i++) {
	//		// Loop to basically continue updating the final composite image
	//		finalComposite = composite2Images(middleman, imageSet[i + 1].img);
	//		string windowTitle = "Composite #" + to_string(i);
	//		if (IMAGE_COMPOSITE_DEBUG) {
	//			namedWindow(windowTitle, WINDOW_NORMAL);
	//			resizeWindow(windowTitle, 600, 600);
	//			imshow(windowTitle, finalComposite);
	//		}
	//		//middleman = finalComposite;
	//		middleman = PadImage(finalComposite);

	//	}

	//	
	//	namedWindow("Final Composite Image", WINDOW_NORMAL);
	//	resizeWindow("Final Composite Image", 600, 600);
	//	imshow("Final Composite Image", middleman);
	//	//PadImage(finalComposite);
	//}
	waitKey(0);
}
