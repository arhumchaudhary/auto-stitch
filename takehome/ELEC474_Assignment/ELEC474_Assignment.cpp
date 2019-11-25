// ELEC474_Assignment.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <opencv2/opencv.hpp>

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

int main()
{
	Mat test = imread("20191119_152011.jpg");
	
	imshow("blah", test);
    std::cout << "Hello World!\n";
	if (STEP3) {
		generateCompositeImage(goodImages);
	}
}

void generateCompositeImage(vector<string> good) {

}