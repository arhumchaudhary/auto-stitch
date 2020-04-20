# Auto Stitch

This program takes a folder of images and creates a panoramic image of the set. 

## Getting Started

### Software Requirements
- OpenCV v4.1+
- CMake v3.10+

### Building
To build the project, navigate to the build folder and run:
````
cmake ..
````
To generate executables:
````
make
````
To run the program on a specified folder, run: 
````
./autostitch <image_folder>
````
This specified folder will be a sub-folder in the images/ folder. 

## Software Pipeline
### Feature Detection
The ORB Feature Detection Module was used to generate matches between image pairs and calculate a fitness score for the match. Pre-processing on the images was done to improve the quality of the images before the feature detection algorithm. This included adding padding and computing affine transformations for the images. The euclidian distance of the matching points was used to determine the fitness score between images, helping the composition algorithm with ordering the images.

### Composition
The composition process uses the image with the highest fitness score to be the centering image. A helper function is then used to merge each image with the composite image by calculating homography matrices and performing the desired geometric transformations. This process is called recursively over the entire image folder, building a final composite image. A back-tracking function is used to verify that all images with low fitness scores are included if the fitness score improves with more images. 

## Results 
The results of the software on the St. James church can be seen below:

![alt text](https://github.com/bchampp/auto-stitch/images/st-james-result.jpg)

## Authors
- [Brent Champion](www.github.com/bchampp)
- [Nathaniel Pauze](www.github.com/natpauze)

## Usage
All code that has been provided is as-is. The authors are not responsible for any actions or consequences associated with the use, distribution and/or modification of these files. This project is for personal, educational use only. By using any code contained in these files, you agree to these terms and will not hold the authors responsible for any actions. 

## License
This project is licensed under the MIT License - see the [LICENSE.md](github.com/bchampp/auto-stitch/LICENSEv) file for details