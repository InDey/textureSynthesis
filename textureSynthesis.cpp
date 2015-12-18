// textureSynthesis.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "synthesis.cpp"
#include <time.h>


int main(int argc, char** argv)
{
	// Get variables
	if (argc != 4) // change argc comparison to 5 if you with wdith and heigth of output
	{
		cout << " Usage: display_image ImageToLoadAndDisplay" << endl;
		return -1;
	}

	Mat img1 = imread(argv[1], IMREAD_COLOR); // Read the file
	if (!img1.data) // Check for invalid input
	{
		cout << "Could not open or find the image" << std::endl;
		return -1;
	}
	Mat txt(img1.size(), CV_32F);
	img1.convertTo(txt, CV_32F);
	txtSz = txt.size();

	texton = strtol(argv[2],NULL,10);
	padding = texton / 6;
	if (texton > txtSz.height && texton > txtSz.width && texton < padding) {
		cout << "Texton is an incorrect size" << std::endl;
		return -1;
	}
	int dist = texton - padding;

	int xdim = strtol(argv[3], NULL, 10);
	int ydim = xdim; // strtol(argv[4], NULL, 10); use this comment to include hieght in cmd line input
	printf("dimentions before: %dx%d\n", xdim, ydim);
	xdim -= xdim%dist;
	ydim -= ydim%dist;

	Size synSz = Size(xdim, ydim);
	int ht = ydim / dist;
	int wd = xdim / dist;
	printf("texton size: %d\nsynthesis dimensions: %dx%d\ntexture dimensions: %dx%d\n", texton, xdim, ydim, txtSz.width, txtSz.height);
	printf("padding: %d\n", padding);


	// setup basic synthesised texture dimensions
	Mat synth(ydim, xdim, CV_32FC3, Scalar(0));

	// choose random starting texton range in texture to start top left of synth texture
	srand(time(NULL));
	int x = rand() % (txtSz.width - texton);
	int y = rand() % (txtSz.height - texton);
	x = (x < 0 ? 0 : x);
	y = (y < 0 ? 0 : y);
	Mat subtxt(txt, cv::Rect(x, y, texton, texton));
	Mat subsynth = Mat(synth, cv::Rect(0, 0, texton, texton));
	subtxt.copyTo(subsynth);

	// pic stitch left to right, top to bottom
		// select next texton
		// calculate padding error
		// calculate min cost path
	for (int j = 0; j < ht; j++) {
		int height = (synSz.height < j*dist + texton ? (synSz.height - j*dist) : texton);
		for (int i = 0; i < wd; i++) {
			int width = (synSz.width < i*dist + texton ? (synSz.width - i*dist) : texton);
			picMatch(txt, synth, width, height, i*dist, j*dist);
		}
		printf("completed row %d of %d\n", j+1, synSz.height / dist);
	}
	

	txt.convertTo(txt, CV_8U);
	synth.convertTo(synth, CV_8U);
	// write to file
	imwrite("synth.bmp", synth);
	// show comparison of texture and synthesized image
	namedWindow("Texture Patch", WINDOW_AUTOSIZE); // Create a window for display.
	imshow("Texture Patch", txt); // Show our image inside it.
	namedWindow("Synthesized Texture", WINDOW_AUTOSIZE); // Create a window for display.
	imshow("Synthesized Texture", synth); // Show our image inside it.
	waitKey(0); // Wait for a keystroke in the window	
	return 0;
}

