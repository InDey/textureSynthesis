
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <math.h>
#include <list>

using namespace cv;
using namespace std;

// Globals
int texton; // texton size
int padding; // overapping thickness
Size txtSz; // dimention of texture

// linked list class
class node {
public:
	int x;
	int y;
	float cost;
	node *next;
};

// funciton prototyping
float minErr(Mat, Mat);
vector<Mat> shrink(list<Mat>, list<float>*);
void picStitchVert(Mat, Mat, int, int, int, int);
void picStitchHoriz(Mat, Mat, int, int, int, int);
node* minCostCutVert(node*, Mat, int, int);
node* minCostCutHoriz(node*, Mat, int, int);

// loops through full texture comparing overlapping patches for minimal difference
void picMatch(Mat txt, Mat synth, int width, int height, int x, int y) {
	// local variables
	list<Mat> picks = list<Mat>();
	list<float> errors = list<float>();
	float tCheck = 999999;
	float lCheck = 999999;
	float check = 999999;
	Mat subSynth(synth, cv::Rect(x, y, width, height));
	Mat slPad(subSynth, Rect(0, 0, padding, height)); // synthesize left padding for comparison
	Mat sbPad(subSynth, Rect(0, 0, width, padding)); // synthesize top padding for comparison

	// goes through full texure considering texton dementions
	for (int j = 0; j < txtSz.height - height; j++) {
		for (int i = 0; i < txtSz.width - width; i++) {
			// get padding in texture to compare to synthesized section
			Mat subtxt(txt, Rect(i, j, width, height));
			Mat tlPad(subtxt, Rect(0, 0, padding, height)); // subtexture left padding
			Mat ttPad(subtxt, Rect(0, 0, width, padding)); // subtexture top padding
			// compare the errors
			float lerror = minErr(tlPad, slPad);
			float terror = minErr(ttPad, sbPad);

			// if patch will be copied to an edge only do comparisons perpendicular to edge
			if (y < padding) {
				if (lerror < lCheck) {
					lCheck = lerror;
					errors.push_front(lerror);
					picks.push_front(subtxt);
				}
			} else if(x < padding) {
				if (terror < tCheck) {
					tCheck = terror;
					errors.push_front(terror);
					picks.push_front(subtxt);
				}
			}
			else {
				float error = lerror + terror;
				if (error < check) {
					check = error;
					errors.push_front(error);
					picks.push_front(subtxt);
				}
			}
		}
	}
	// shrink list to only subtextures < 10% more than the smallest value
	vector<Mat> list = shrink(picks, &errors);
	
	// pick random from remaining subtextures
	int pos = rand() % list.size();
	Mat patch(width, height, CV_32FC3);
	
	// copy the subtexture to a sparate patch for cross patch stitching
	list.at(pos).copyTo(patch);
	if (x < padding) {
		picStitchHoriz(patch, synth, width, height, x, y);
	}
	else if (y < padding) {
		picStitchVert(patch, synth, width, height, x, y);
	}
	else {
		picStitchVert(patch, synth, width, height, x, y);
		picStitchHoriz(patch, synth, width, height, x, y);
	}
	patch.copyTo(subSynth);
}

// finds the min error between 2 patches
float minErr(Mat one, Mat two) {
	Mat diff = two - one;
	diff = diff.mul(diff);
	return sqrtf(cv::sum(diff)[0] + cv::sum(diff)[1] + cv::sum(diff)[2]);
}

// brings best possible choices withing 10% error of min error patch
vector<Mat> shrink(list<Mat> patches, list<float> *errors) {
	vector<Mat> matches = vector<Mat>();
	list<float> errlist = list<float>();
	Mat min = patches.front();
	float error = errors->front();
	matches.push_back(min);
	errlist.push_back(error);
	patches.pop_front();
	errors->pop_front();
	float pct = 0.1f;
	int size = patches.size();

	for (int i = 0; i < size; i++) {
		if (errors->front() < error+error*pct) {
			matches.push_back(patches.front());
			errlist.push_back(errors->front());
		} 
		errors->pop_front();
		patches.pop_front();
	}

	*errors = errlist;
	return matches;
}

// stiches synth into the left padding of the patch found
void picStitchVert(Mat patch, Mat synth, int width, int height, int x, int y) {
	// vertical 
	Mat pPad(patch, cv::Rect(0, 0, padding, height));
	Mat sPad(synth, cv::Rect(x, y, padding, height));
	Mat pad = pPad - sPad;
	node *E = new node();
	float min = 9999999;

	// find starting node
	for (int i = 0; i < pad.cols; i++) {
		cv::Vec3f error = pad.at<cv::Vec3f>(cv::Point(i, 0));
		error.mul(error);
		float err = sqrtf(error[0] + error[1] + error[2]);
		if (err < min) {
			E->x = i;
			min = err;
		}
	}

	// recursivley find the vertical stitch coordinates
	E->cost = min;
	E->y = 0;
	E->next = new node();
	E->next = minCostCutVert(E, pad, height, 1);
	node *root = E;

	// copy each row over based on E.x position
	do{
		int wd = root->x;
		int ht = root->y;
		//fill in padding
		Mat subSynth(synth, cv::Rect(x, y + ht, wd, 1));
		Mat subPatch(patch, cv::Rect(0, ht, wd, 1));
		subSynth.copyTo(subPatch);
		node *temp = root;
		root = root->next;
		free(temp);
		temp = NULL;
	}while (root->next != NULL);
}

// stiches synth into the top padding of the patch found
void picStitchHoriz(Mat patch, Mat synth, int width, int height, int x, int y) {
	// Horizontal
	Mat pPad(patch, cv::Rect(0, 0, width, padding));
	Mat sPad(synth, cv::Rect(x, y, width, padding));
	Mat pad = pPad - sPad;
	node *E = new node();
	float min = 9999999;

	// find starting node
	for (int i = 0; i < pad.rows; i++) {
		cv::Vec3f error = pad.at<cv::Vec3f>(cv::Point(0, i));
		float err = error[0] + error[1] + error[2];
		if (err < min) {
			E->y = i;
			min = err;
		}
	}

	// recursivley find the horizontal stich coordinates
	E->cost = min;
	E->x = 0;
	E->next = new node();
	E->next = minCostCutHoriz(E, pad, width, 1);
	node *root = E;

	// copy each column over based on E.y position
	do {
		int wd = root->x;
		int ht = root->y;
		//fill in padding
		Mat subSynth(synth, cv::Rect(x + wd, y, 1, ht));
		Mat subPatch(patch, cv::Rect(wd, 0, 1, ht));
		subSynth.copyTo(subPatch);
		node *temp = root;
		root = root->next;
		free(temp);
		temp = NULL;
	}while (root != NULL);
}

// find vertical minimum cost cut
node* minCostCutVert(node *e, Mat pad, int N, int i) {
	node *E = new node();
	E->y = i;

	// calculate errors of the next potential posisitons
	// check edge cases and see which intensity is least
	Vec3f pts[3];
	float errs[3] = { 9999999, 9999999, 9999999 };
	if (e->x > 0) {
		pts[0] = pad.at<Vec3f>(cv::Point(e->x - 1, i));
		pts[0] = pts[0].mul(pts[0]);
		errs[0] = sqrtf(pts[0][0] + pts[0][1] + pts[0][2]);
	}
	pts[1] = pad.at<Vec3f>(cv::Point(e->x, i));
	pts[1] = pts[1].mul(pts[1]);
	errs[1] = sqrtf(pts[1][0] + pts[1][1] + pts[1][2]);
	if (e->x < pad.cols - 1) {
		pts[2] = pad.at<Vec3f>(cv::Point(e->x + 1, i));
		pts[2] = pts[2].mul(pts[2]);
		errs[2] = sqrtf(pts[2][0] + pts[2][1] + pts[2][2]);
	}

	// find which y position is cheapest to go to next
	float err = 9999;
	int x;
	for (int j = 0; j < 3; j++) {
		if (errs[j] < err) {
			x = j - 1;
			err = errs[j];
		}
	}

	// calculate total cost and next position
	E->cost = e->cost + err;
	E->x = e->x + x;
	
	// ending step
	if (i == N - 1) {
		E->next = NULL;
		return E;
	}
	//E->next = new node();
	E->next = minCostCutVert(E, pad, N, i + 1);
	return E;
}

// find horizontal minimum cost cut
node* minCostCutHoriz(node *e, Mat pad, int N, int i) {
	node *E = new node();
	E->x = i;

	// calculate errors of the next potential posisitons
	// check edge cases and see which intensity is least
	Vec3f pts[3];
	float errs[3] = { 9999999, 9999999, 9999999 };
	if (e->y > 0) {
		pts[0] = pad.at<Vec3f>(cv::Point(i, e->y - 1));
		pts[0] = pts[0].mul(pts[0]);
		errs[0] = sqrtf(pts[0][0] + pts[0][1] + pts[0][2]);
	}
	pts[1] = pad.at<Vec3f>(cv::Point(i, e->y));
	pts[1] = pts[1].mul(pts[1]);
	errs[1] = sqrtf(pts[1][0] + pts[1][1] + pts[1][2]);
	if (e->y < pad.rows - 1) {
		pts[2] = pad.at<Vec3f>(cv::Point(i, e->y + 1));
		pts[2] = pts[2].mul(pts[2]);
		errs[2] = sqrtf(pts[2][0] + pts[2][1] + pts[2][2]);
	}

	// find which y position is cheapest to go to next
	float err = 9999;
	int y;
	for (int j = 0; j < 3; j++) {
		if (errs[j] < err) {
			y = j - 1;
			err = errs[j];
		}
	}

	// calculate total cost and next position
	E->cost = e->cost + err;
	E->y = e->y + y;

	// ending step
	if (i == N - 1) {
		E->next = NULL;
		return E;
	}
	// recursive call
	E->next = new node();
	E->next = minCostCutHoriz(E, pad, N, i + 1);
	return E;
}

