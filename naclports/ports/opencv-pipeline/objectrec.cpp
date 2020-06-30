#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <openssl/sha.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef __native_client__
#include <request.h>
#endif

#if defined(__native_client__) && defined(_NEWLIB_VERSION)
#include <fts.h>
#endif

#include "pipeline/cert.h"
#include "pipeline/errors.h"
#include "pipeline/worker.h"
#include "pipeline/pipeline.h"
#include "pipeline/json-builder.h"

#include "tagger.h"

const char* prog_name = "TAG_IMAGE";

using namespace std;
using namespace cv;

#ifndef CASCADE_DIR
#define CASCADE_DIR "models/"
#endif

#define xstr(s) to_str(s)
#define to_str(s) #s

#define HASHLEN SHA256_DIGEST_LENGTH

Tagger img_tagger;

void gen_tags(Mat& frame, TagSet &tags)
{
    img_tagger.tag_photo(frame, tags);
}

/* write hash as a hex number (so we need two characters for every byte */
char *hash_str(const unsigned char hash[HASHLEN], char out[HASHLEN * 2 + 1])
{
    for (int i = 0; i < HASHLEN; ++i) {
        sprintf(out + i * 2, "%02x", hash[i]);
    }
    out[HASHLEN *2] = '\0';
    return out;
}

void write_to_stream(ostream &out, const unsigned char hash[HASHLEN],
        const TagSet &tags)
{
    char hash_out[HASHLEN * 2 + 1];
    out << "{\"hash\":\"" << hash_str(hash, hash_out) << "\",";
    out <<"\"tags\":[";
    for (const auto& tag : tags) {
       out << '"' << tag << '"';
       if (&tag != &tags.back()){
          out << ",";
       }
    }
    out << "]}" << endl;
}

void output(const unsigned char hash[HASHLEN], const TagSet &tags,
        const vector<unsigned char>& buffer, work_ctx ctx)
{
    if (!ctx) {
        write_to_stream(cout, hash, tags);
    } else {
        ostringstream out;
        write_to_stream(out, hash, tags);
        string str(out.str());
        put_work_desc(ctx, str.data(), str.length() + 1, buffer.data(),
                buffer.size());
    }
}

int add_model_at(const string& dirname, const char* filename)
{
    const char *model_name = strchr(filename, '_');
    int len;

    /* check valid and move to first char after _ */
    if (! model_name++) {
        return -1;
    }

    len = strchrnul(model_name, '.') - model_name;

    if(!img_tagger.add_model(string(model_name, len), dirname + filename)) {
        return -1;
    }
    return 0;
}

#if defined(__native_client__) && defined(_NEWLIB_VERSION)
int compare(const FTSENT* const * a, const FTSENT* const * b) {
    return (strcmp((*a)->fts_name, (*b)->fts_name));
}
#endif

int load_models()
{
    DIR *dir;
    const struct dirent *dp;
    int ret = -1;
    string dirname(xstr(CASCADE_DIR));

#if defined(__native_client__) && defined(_NEWLIB_VERSION)
    FTS* file_system = NULL;
    FTSENT *node     = NULL;
    char* dir_name   = (char*)xstr(CASCADE_DIR);
    file_system = fts_open(&dir_name,
                           FTS_COMFOLLOW|FTS_NOCHDIR, &compare);

    if (NULL != file_system) {
      while ( (node = fts_read(file_system)) != NULL ) {
        switch (node->fts_info) {
        case FTS_NSOK:
          cerr << "A file for which no stat(2) information was requested."
               << endl;
        case FTS_F:
          if (add_model_at(dirname, node->fts_name)) {
            cerr << "--(!)Error loading cascade: " << dp->d_name
                 << endl;
            fts_close(file_system);
            return ret;
          }
          break;
        case FTS_DNR:
        case FTS_NS:
        case FTS_ERR:
          perror("fts_read");
          fts_close(file_system);
          return ret;
        default:
          break;
        }
      }
      fts_close(file_system);
      ret = 0;
    }

    return ret;
#else
    /* Load all models in the models subdir for now */
    dir = opendir(xstr(CASCADE_DIR));
    if (!dir) {
        perror("couldn't open " xstr(CASCADE_DIR));
        goto done;
    }
    do {
        if ((dp = readdir(dir))) {
            switch (dp->d_type) {
            case DT_DIR:
                if (strcmp(".", dp->d_name) == 0)
                    break;
                if (strcmp("..", dp->d_name) == 0)
                    break;
            case DT_BLK:
            case DT_UNKNOWN:
                cerr << "refusing to open model: " << dp->d_name << endl;
                break;
            default:
                if (add_model_at(dirname, dp->d_name)) {
                    cerr << "--(!)Error loading cascade: " << dp->d_name
                         << endl;
                    goto done;
                }
            }
        }
    } while (dp != NULL);

    /* all good */
    ret = 0;

 done:
    closedir(dir);
    return ret;
#endif
}

int load_model(string &tag, string &model_path)
{
    if(!img_tagger.add_model(tag, model_path)) {
        return -1;
    }
    return 0;
}

int hash_file(unsigned char hash[HASHLEN], int fd, vector<unsigned char> &buf)
{
    SHA256_CTX ctx;

    if (SHA256_Init(&ctx) != 1) {
        return -1;
    }

    if (buf.size() == 0) {
        unsigned char buffer[512];
        int64_t bytes_read;
        do {
            bytes_read = read(fd, buffer, 512);
            if (bytes_read < 0) {
                perror("could not hash file");
                return -1;
            }
            if (bytes_read) {
                if (SHA256_Update(&ctx, buffer, bytes_read) != 1) {
                    return -1;
                }
                buf.insert(buf.end(), buffer, buffer + bytes_read);
            }
        } while (bytes_read);
    } else {
        if (SHA256_Update(&ctx, buf.data(), buf.size()) != 1) {
            return -1;
        }
    }

    if (SHA256_Final(hash, &ctx) != 1) {
        return -1;
    }
    return 0;
}

int do_work(int img_fd, json_value *desc, vector<unsigned char>& buffer,
        work_ctx ctx=NULL)
{
    Mat image;
    TagSet tags;
    unsigned char hash[HASHLEN];

    if (hash_file(hash, img_fd, buffer)) {
        cerr << "could not hash" << endl;
    }

    image = imdecode(buffer, 1);

    gen_tags(image, tags);
    output(hash, tags, buffer, ctx);

    return 0;
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
    int ret = 0;
    int not_ready;

    settings.value_extra = json_builder_extra;

#ifdef __native_client__
    while (wait_for_chan(false, NULL, NULL, NULL, NULL, NULL) >= 0)
#else
    while (1)
#endif
    {
#ifdef __native_client__
        while ((not_ready = get_work_desc(ctx, &desc, &dlen, &data, &len)) >= 0) {
            if (not_ready) continue;
#else
        while (get_work_desc_single(ctx, &desc, &dlen, &data, &len) >= 0) {
#endif
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
    }

    return ret;
}

int main(int argc, char* argv[])
{
    char *image_name;
    vector<unsigned char> buffer;
    WorkSpec *spec;
    int ret;

    if (argc < 4) {
        cerr << "Image tagger: not enough arugments" << endl;
        return 1;
    }

    string name(argv[1]);
    string path(argv[2]);
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
