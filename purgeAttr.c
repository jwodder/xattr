/* purgeAttr.c - a program for getting rid of Mac OS X's extended attributes
 * Written 7 Feb 2008 by John T. Wodder II
 * Last edited 14 Feb 2008 by John Wodder
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/xattr.h>

int isadir(const char* path);
int issymlink(const char* path);
void purgeAttr(const char* path, _Bool slink);
void purgeDir(char* path);
void usage(void);

struct {_Bool verbose, recurse: 1; char slink; } flags;

#define SLINK_CLONLY 0  /* Only follow symlinks on the command line (default) */
#define SLINK_ALL 1  /* Follow all symbolic links */
#define SLINK_NONE 2  /* No symbolic links are followed */

int main(int argc, char** argv) {
 int ch;
 while ((ch = getopt(argc, argv, "vrRHPL")) != -1) {
  switch (ch) {
   case 'v': flags.verbose = 1; break;
   case 'r': case 'R': flags.recurse = 1; break;
   case 'H': flags.slink = SLINK_CLONLY; break;
   case 'P': flags.slink = SLINK_NONE; break;
   case 'L': flags.slink = SLINK_ALL; break;
   default: usage();
  }
 }
 argc -= optind;
 argv += optind;
 if (argc < 1) usage();
 for (int i=0; i<argc; i++) {
  purgeAttr(argv[i], flags.slink != SLINK_NONE);
  if (flags.recurse && isadir(argv[i]) > 0 && (flags.slink != SLINK_NONE || issymlink(argv[i]) == 0)) purgeDir(argv[i]);
 }
 return 0;
}

int isadir(const char* path) {
 struct stat dat;
 if (stat(path, &dat) != 0) {
  fprintf(stderr, "purgeAttr: %s: ", path); perror(NULL); return -1;
 }
 return S_ISDIR(dat.st_mode);
}

int issymlink(const char* path) {
 struct stat dat;
 if (lstat(path, &dat) != 0) {
  fprintf(stderr, "purgeAttr: %s: ", path); perror(NULL); return -1;
 }
 return S_ISLNK(dat.st_mode);
}

void purgeAttr(const char* path, _Bool slink) {
 int attrSize = listxattr(path, NULL, 0, slink ? XATTR_NOFOLLOW : 0);
 if (attrSize == 0) return;
 else if (attrSize == -1) {
  fprintf(stderr, "purgeAttr: error getting attributes for %s: ", path);
  perror(NULL); return;
 }
 char* buffer = calloc(attrSize, sizeof(char));
 if (!buffer) {fprintf(stderr, "purgeAttr: out of memory\n"); exit(1); }
 int outLen = listxattr(path, buffer, attrSize, slink ? XATTR_NOFOLLOW : 0);
 if (outLen == 0) {free(buffer); return; }
 else if (outLen == -1) {
 /* Check if errno == ERANGE ? */
  fprintf(stderr, "purgeAttr: error getting attributes for %s: ", path);
  perror(NULL); return;
 }
 char* attrName = buffer;
 while (outLen > 0) {
  int success = removexattr(path, attrName, slink ? XATTR_NOFOLLOW : 0);
  if (success == 0 && flags.verbose) printf("%s: %s removed\n", path, attrName);
  else if (success == -1) {
   fprintf(stderr, "purgeAttr: error removing %s from %s: ", attrName, path);
   perror(NULL);
  }
  char* next = strchr(attrName, '\0');
  if (next == NULL) break;  /* Just in case */
  outLen -= ++next - attrName;
  attrName = next;
 }
 free(buffer);
}

void purgeDir(char* path) {
 DIR* dir = opendir(path);
 if (dir == NULL) {
  fprintf(stderr, "purgeAttr: %s: ", path); perror(NULL); return;
 }
 size_t len = strlen(path);
 struct dirent* entry;
 while ((entry = readdir(dir)) != NULL) {
  if (entry->d_namlen < 3 && entry->d_name[0] == '.' && (entry->d_namlen < 2 ||
   entry->d_name[1] == '.')) continue;
  /* There are no zero-length filenames, right? */
  char* subPath = calloc(len+entry->d_namlen+2, sizeof(char));
  if (subPath == NULL) {perror("purgeAttr"); exit(1); }
  strncpy(subPath, path, len);
  subPath[len] = '/';
  strncat(subPath, entry->d_name, entry->d_namlen);
  purgeAttr(subPath, flags.slink == SLINK_ALL);
  if (isadir(subPath) > 0 && (flags.slink == SLINK_ALL || issymlink(subPath) == 0)) purgeDir(subPath);
  free(subPath);
 }
 closedir(dir);
}

void usage(void) {
 fprintf(stderr, "Usage: purgeAttr [-H | -L | -P] [-Rv] files ...\n");
 exit(2);
}
