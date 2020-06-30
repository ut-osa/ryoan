#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <openssl/sha.h>
#include <iostream>
#include <vector>
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

#include "util.hpp"

#ifndef CASCADE_DIR
#define CASCADE_DIR "models/"
#endif

using namespace std;
using namespace cv;

const char* prog_name = "BLUR_IMAGE";
CascadeClassifier model;
string model_name;
string output_path;

void write_to_stream(ostream &out, const unsigned char hash[HASHLEN],
        string& name, int count)
{
    char hash_out[HASHLEN * 2 + 1];
    out << "{\"hash\":\"" << hash_str(hash, hash_out) << "\",";
    out << "\"name\":";
    out << "\"" << name << "\","; 
    out << "\"blurred\":";
    out << count;
    out << "}" << endl;
}

void output(const unsigned char hash[HASHLEN], string& name, int count,
        const vector<unsigned char>& buffer, work_ctx ctx)
{
    if (!ctx) {
        write_to_stream(cout, hash, name, count);
    } else {
        ostringstream out;
        write_to_stream(out, hash, name, count);
        string str(out.str());
        put_work_desc(ctx, str.data(), str.length() + 1, buffer.data(),
                buffer.size());
    }
}

int load_model(string& name, string &mp) 
{
    model_name = name;
    if(! model.load(mp) ){
        return -1;
    }
    return 0;
}


int do_work(int img_fd, json_value *desc, vector<unsigned char>& buffer,
        work_ctx ctx=NULL)
{
    Mat image;
    unsigned char hash[HASHLEN];

    vector<Rect> faces;
    Mat image_gray;

    if (hash_file(hash, img_fd, buffer)) {
        cerr << "could not hash" << endl;
    }

    image = imdecode(buffer, 1);

    cvtColor( image, image_gray, COLOR_BGR2GRAY );
    equalizeHist( image_gray, image_gray );
    model.detectMultiScale( 
        image_gray, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE);

    for( uint64_t i = 0; i < faces.size(); i++ )
    {
        Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
        ellipse( image, center, 
            Size( faces[i].width*0.5, faces[i].height*0.5), 
            0, 0, 360, Scalar( 255, 255, 255 ), 4, 8, 0 );

        Mat faceROI = image(faces[i]);
        int width = faces[i].width % 2 == 0 ? faces[i].width + 1 : faces[i].width;
        int height = faces[i].height % 2 == 0 ? faces[i].height + 1 : faces[i].height;
        GaussianBlur(faceROI, faceROI, Size(width, height), 0);
    }

    output(hash, model_name, faces.size(), buffer, ctx);

    return imwrite(output_path, image);
}

int run_pipeline()
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
            ret = do_work(-1, jdesc, data_vec, ctx);
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
    int ret;

    if (argc < 5) {
        cerr << "Image tagger: not enough arugments" << endl;
        return 1;
    }
    string name(argv[1]);
    string path(argv[2]);
    output_path = string(argv[3]);
    if (load_model(name, path)) {
        cerr << "model issue" << endl;
        return 1;
    }

    spec = WorkSpec_parse(argv[argc - 1]);

    if (spec) {
        if (setup_for_work(spec)) {
            error("setup failed");
            return 1;
        }
        assert(spec->n > 0);
        ret = run_pipeline();
        finish_work(1);
    } else {
        if (argc < 2) {
           cerr << "--(!)Error no image input\n";
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
