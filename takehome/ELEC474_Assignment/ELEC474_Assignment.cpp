// ELEC474_Assignment.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp> // OpenCV Core Functionality
#include <opencv2/highgui/highgui.hpp> // High-Level Graphical User Interface


using namespace std;
using namespace cv;


/* ELEC474 Take home exam
Things to do:
- Recognize the images as 'sets' -> some functionality to sift through a folder
- Feature extraction on each set
- Figure out a metric for measuring each set
	- SIFT, ORB, SURF etc
- Develop match metric to group good images together
	- Maybe smething that will basically give each one a score relative to how many other images make good matches
	we could do some cool kinda graph stuff as well to basically figure out which grouping has the best score
	- INITAL THOUGHTS:
		- Iterate over a folder of images -> basically pick one and build a graph from it to the other images
		- Try to find a circle of images with the best overall match to eachother -> store in a 'good images' folder
		- 
2. Figure out the transformation between these good image pairs
3. Write the image composition algorithm*/	

#define STEP1 1
#define STEP2 1
#define STEP3 1


vector<string> goodImages;

//Class for storing an image, the img mat, name, features etc go in a lsit of this type
class ourImage {
public:
	string path;
	string name;
	Mat img;
	Mat imgGrey;


	ourImage( string path) {
		this->path = path;
		this->img = imread(path);
		cvtColor(this->img, this->imgGrey, COLOR_RGB2GRAY);
	}
};
//set of the images 
list<ourImage> imageSet;




/* Function Protocols */
void generateCompositeImage(vector<string> good); // Final output

int main(int argc, char* argv[])
{

	cout << "current program running from direcotry" << endl;
	cout << filesystem::current_path() << endl;


	Mat test = imread("20191119_152011.jpg");
	if (!test.empty()) {
		imshow("blah", test);
	}
	else {
		cout << "No image found";
	}
	//open all images in a folder
	string folderPath = "office2";
	try {
		for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
			std::cout << entry.path() << std::endl;
			imageSet.push_back(entry.path().string());//c++ is so weird, i just need to pass in the construtor params and not a new instance 
		}
	}
	catch (const std::exception & e) { 
		//probably couldnt find the folder
		cout << e.what() << endl;
	}
	for (ourImage img1 : imageSet) {
		//show the grayscale 
		imshow(img1.path, img1.imgGrey);
	}

	waitKey();



    std::cout << "Hello World!\n";
	if (STEP3) {
		generateCompositeImage(goodImages);
	}
}

void generateCompositeImage(vector<string> good) {
	return;
}