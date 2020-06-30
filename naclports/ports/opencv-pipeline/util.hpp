#include <openssl/sha.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

using namespace std;

#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#define HASHLEN SHA256_DIGEST_LENGTH

/* write hash as a hex number (so we need two characters for every byte */
char *hash_str(const unsigned char hash[HASHLEN], char out[HASHLEN * 2 + 1]);

int hash_file(unsigned char hash[HASHLEN], int fd, vector<unsigned char> &buf);

#endif