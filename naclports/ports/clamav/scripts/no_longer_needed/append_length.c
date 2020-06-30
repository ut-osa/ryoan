/*
 * =====================================================================================
 *
 *       Filename:  append_length.c
 *
 *    Description:  append length of a file at the beginning (unsigned int), then output
 *                  to stdout.
 *
 *        Version:  1.0
 *        Created:  03/28/2016 05:56:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc, const char *argv[])
{
  struct stat st;
  int fd;
  unsigned int len;
  char buf[4096];
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return -1;
  }
  if (stat(argv[1], &st)) {
    fprintf(stderr, "Error: stat %s\n", argv[1]);
    return -1;
  }
  len = (unsigned int)st.st_size;

  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Error: open %s\n", argv[1]);
    return -1;
  }
  write(STDOUT_FILENO, &len, sizeof(len));
  while ((len = read(fd, buf, sizeof(buf))) > 0) {
    write(STDOUT_FILENO, buf, len);
  }
  close(fd);
  return 0;
}
