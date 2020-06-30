#include "tagger.h"

using namespace cv;

bool Tagger::add_model(const std::string &name, const std::string &filename)
{
   counters.emplace_back(name);

   ObjectCounter& counter = counters.back();

   bool ret = counter.load(filename);
   if (!ret) {
      counters.pop_back();
   }
   return ret;
}

void Tagger::tag_photo(const Mat& img, TagSet& tags)
{
   for (const auto &counter : counters) {
      if (counter.count(img) > 0) {
         tags.emplace_back(counter.get_name());
      }
   }
}
