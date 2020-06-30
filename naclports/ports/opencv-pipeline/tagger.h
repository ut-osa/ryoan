#ifndef _TAGGER_H_
#define _TAGGER_H_

#include "objectcounter.h"

typedef std::string tag;
typedef std::vector<tag> TagSet;
class Tagger {
private:
   std::vector<ObjectCounter> counters;
public:
   bool add_model(const std::string &name, const std::string &filename);
   void tag_photo(const cv::Mat&, TagSet&);
};

#endif
