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

const char* prog_name = "NORMALIZE_IMAGE";

using namespace std;
using namespace cv;

bool do_normalize(Mat base, const char* dest){
    Mat result(base);
    normalize(base, result, 0, 255, NORM_MINMAX);
    // write image to disk
    return imwrite(dest, result);
}


void write_to_stream(ostream &out, const unsigned char hash[HASHLEN],
        const unsigned char hash_res[HASHLEN],
        const char* path)
{
    char hash_out[HASHLEN * 2 + 1];
    out << "{\"hash\":\"" << hash_str(hash, hash_out) << "\",";
    out << " \"result\": \"" << hash_str(hash_res, hash_out) << "\",";
    out << " \"path\": \"" << path << "\"";
    out << "}" << endl;
}

void output(const unsigned char hash[HASHLEN], 
        const unsigned char hash_res[HASHLEN],
        const char* path, const vector<unsigned char>& buffer, work_ctx ctx)
{
    if (!ctx) {
        write_to_stream(cout, hash, hash_res, path);
    } else {
        ostringstream out;
        write_to_stream(out, hash, hash_res, path);
        string str(out.str());
        put_work_desc(ctx, str.data(), str.length() + 1, buffer.data(),
                buffer.size());
    }
}

int do_work(int img_fd, json_value *desc, vector<unsigned char>& buffer,
        const char* dest, work_ctx ctx=NULL)
{
    Mat image;

    unsigned char hash[HASHLEN];
    unsigned char hash_res[HASHLEN];

    if (hash_file(hash, img_fd, buffer)) {
        cerr << "could not hash" << endl;
    }

    image = imdecode(buffer, 1);

    if(! do_normalize(image, dest)){
        return -1;
    }

    int fd_res = open(dest, O_RDONLY);
    auto temp = vector<unsigned char>();
    if (hash_file(hash_res, fd_res, temp)) {
        cerr << "could not hash" << endl;
    }
    output(hash, hash_res, dest, buffer, ctx);

    return 0;
}

int run_pipeline(const char* dest)
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
            ret = do_work(-1, jdesc, data_vec, dest, ctx);
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

    string dest(argv[1]);

    spec = WorkSpec_parse(argv[argc - 1]);

    if (spec) {
        if (setup_for_work(spec)) {
            cerr << "setup failed" << endl;
            return 1;
        }
        assert(spec->n > 0);
        ret = run_pipeline(dest.c_str());
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
