#include <iostream>
#include "opencv2/imgproc.hpp"
#include "objectcounter.h"

using namespace std;
using namespace cv;

bool ObjectCounter::load(const string& model_file)
{
   return classifier->load(model_file);
}

unsigned long ObjectCounter::count(const Mat& img) const
{
   vector<Rect> objects;
   Mat img_gray;

   cvtColor(img, img_gray, COLOR_BGR2GRAY);
   equalizeHist(img_gray, img_gray);

   classifier->detectMultiScale(img_gray, objects, 1.1, 2,
         0|CASCADE_SCALE_IMAGE, Size(30, 30));

   return objects.size();
}


