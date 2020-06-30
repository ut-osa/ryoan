#include "util.hpp"

/* write hash as a hex number (so we need two characters for every byte */
char *hash_str(const unsigned char hash[HASHLEN], char out[HASHLEN * 2 + 1])
{
    for (int i = 0; i < HASHLEN; ++i) {
        sprintf(out + i * 2, "%02x", hash[i]);
    }
    out[HASHLEN *2] = '\0';
    return out;
}

int hash_file(unsigned char hash[HASHLEN], int fd, vector<unsigned char> &buf) {
    SHA256_CTX ctx;

    if (SHA256_Init(&ctx) != 1) {
        return -1;
    }

    if (buf.size() == 0) {
        unsigned char buffer[512];
        ssize_t bytes_read;
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