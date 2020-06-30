#include <opencv2/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <openssl/sha.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#include "pipeline/cert.h"
#include "pipeline/errors.h"
#include "pipeline/worker.h"
#include "pipeline/pipeline.h"
#include "pipeline/json-builder.h"

#include <assert.h>

#include "util.hpp"

const char* prog_name = "SEARCH_IMAGE";

using namespace std;
using namespace cv;

int do_search(Mat img, Mat templ){
    Mat result, gimg, gtempl;
    int result_cols =  img.cols - templ.cols + 1;
    int result_rows = img.rows - templ.rows + 1;
    double min_value = 0.9, max_value = 0.9, t_hold = 0.9;
    int count = 0;

    cvtColor(img, gimg, COLOR_BGR2GRAY);
    cvtColor(templ, gtempl, COLOR_BGR2GRAY);

    result.create( result_rows, result_cols, CV_32FC1 );
    matchTemplate( gimg, gtempl, result, TM_CCOEFF_NORMED);
    threshold( result, result, 0.9, 1., THRESH_TOZERO);

    while (max_value >= t_hold) {
        Point minloc, maxloc;
        minMaxLoc(result, &min_value, &max_value, &minloc, &maxloc);
 
        if (max_value >= t_hold) {
            rectangle(
                        img,
                        maxloc,
                        Point(maxloc.x + templ.cols, maxloc.y + templ.rows),
                        Scalar(0,255,0), 2
                        );
            floodFill(result, maxloc,
                    Scalar(0), 0, Scalar(.1), Scalar(1.));
            ++count;
        }
    }

    return count;
}

void write_to_stream(ostream &out, const unsigned char hash[HASHLEN],
        const unsigned char hash_tmpl[HASHLEN], int occurrences)
{
    char hash_out[HASHLEN * 2 + 1];
    out << "{\"hash\":\"" << hash_str(hash, hash_out) << "\",";
    out << " \"template\": [{ \"hash\": \"" << hash_str(hash_tmpl, hash_out) << "\",";
    out <<"\"occurrences\": ";
    out << occurrences;
    out << "}]}" << endl;
}

void output(const unsigned char hash[HASHLEN], 
        const unsigned char hash_tmpl[HASHLEN],
        int occurrences, const vector<unsigned char>& buffer, work_ctx ctx)
{
    if (!ctx) {
        write_to_stream(cout, hash, hash_tmpl, occurrences);
    } else {
        ostringstream out;
        write_to_stream(out, hash, hash_tmpl, occurrences);
        string str(out.str());
        put_work_desc(ctx, str.data(), str.length() + 1, buffer.data(),
                buffer.size());
    }
}

int do_work(int img_fd, json_value *desc, vector<unsigned char>& buffer,
        Mat& image_key, work_ctx ctx=NULL)
{
    Mat image;
    int occurrences;

    unsigned char hash[HASHLEN];
    unsigned char hash_tmpl[HASHLEN];
    vector<unsigned char> key_buf;
    unsigned char *input = (unsigned char*)(image_key.data);

    if (hash_file(hash, img_fd, buffer)) {
        cerr << "could not hash" << endl;
    }

    for(int j = 0;j < image_key.rows;j++){
        for(int i = 0;i < image_key.cols;i++){
            key_buf.push_back(input[image_key.step * j + i ]);
            key_buf.push_back(input[image_key.step * j + i + 1]);
            key_buf.push_back(input[image_key.step * j + i + 2]);
        }
    }

    if (hash_file(hash_tmpl, -1, key_buf)) {
        cerr << "could not hash" << endl;
    }

    image = imdecode(buffer, 1);

    occurrences = do_search(image, image_key);
    output(hash, hash_tmpl, occurrences, buffer, ctx);

    return 0;
}

int run_pipeline(Mat& image_key)
{
    unsigned char *data;
    char *desc;
    work_ctx ctx = alloc_ctx();
    int64_t len, dlen;
    json_settings settings = {0};
    json_value *jdesc;
    char error[128];
    int ret = -1;
    int not_ready;

    settings.value_extra = json_builder_extra;

    while ((not_ready = get_work_desc(ctx, &desc, &dlen, &data, &len)) >= 0) {
        if (not_ready) continue;
        vector<unsigned char> data_vec(data, data + len);
        if ((jdesc = json_parse_ex(&settings, desc, len, error))) {
            ret = do_work(-1, jdesc, data_vec, image_key, ctx);
        } else {
            eprintf("XXX JSON parsing failed: %s\n", error);
        }
        json_builder_free(jdesc);
        free(data);
        free(desc);
        if (ret) break;
    }

    return ret;
}

int main(int argc, char* argv[])
{
    char *image_name;
    vector<unsigned char> buffer;
    WorkSpec *spec;
    Mat image_key;
    int ret;

    if (argc < 2) {
        cerr << "Image searcher: not enough arugments" << endl;
        return 1;
    }

    string path(argv[1]);
    image_key = imread(path);

    if (image_key.data == NULL) {
        cerr << "image read issue: " << path << endl;
        return 1;
    }

    spec = WorkSpec_parse(argv[argc - 1]);

    if (spec) {
        if (setup_for_work(spec)) {
            cerr << "setup failed" << endl;
            return 1;
        }
        assert(spec->n > 0);
        ret = run_pipeline(image_key);
        finish_work(1);
    } else {
        if (argc < 2) {
           cerr << "--(!)Error no image input" << endl;
           return -1;
        }
        int img_fd;
        image_name = argv[1];
        img_fd = open(image_name, O_RDONLY);
        if (img_fd < 0) {
            perror("Could not open image file");
            return 1;
        }
        close(img_fd);
    }

    return ret;
}
