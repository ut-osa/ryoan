#ifndef _OBJECT_COUNTER_H_
#define _OBJECT_COUNTER_H_
#include "opencv2/objdetect.hpp"

#include <memory>

typedef std::unique_ptr<cv::CascadeClassifier> Classifier_ptr;

class ObjectCounter {
private:
   Classifier_ptr classifier;
   std::string name;
public:
   ObjectCounter(const std::string &_name) :
      classifier(new cv::CascadeClassifier), name(_name) {}
   unsigned long count(const cv::Mat&) const;
   bool load(const std::string &model_file);
   std::string get_name() const { return name; }
};

#endif
