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

vector<string> imageSet;
vector<string> goodImages;



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
	//string folderPath = "E:\\school\\ELEC474\\ELEC474-Exam\\takehome\\ELEC474_Assignment\\office2";
	string folderPath = "office2";
	
	try {
		for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
			std::cout << entry.path() << std::endl;
		}
	}
	catch (const std::exception & e) { 
		cout << e.what() << endl;
	}





    std::cout << "Hello World!\n";
	if (STEP3) {
		generateCompositeImage(goodImages);
	}
}

void generateCompositeImage(vector<string> good) {
	return;
}