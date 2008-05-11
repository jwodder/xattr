/* To implement:
 - attribute removal
 - printing of attribute size?
 - switch to not follow symlinks
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/xattr.h>

#define checkMem(p)  if (!p) {perror("xattr"); exit(1); }

struct {_Bool v, x : 1; enum {list=0, set} mode; } flags;

void usage(_Bool verbose);

int main(int argc, char** argv) {
 int ch;
 while ((ch = getopt(argc, argv, "lsvh")) != -1) {
  switch (ch) {
   case 'l': flags.mode = list; break;
   case 's': flags.mode = set; break;
   case 'v': flags.v = 1; break;
   case 'h': usage(1); return 0;
   case 'x': flags.x = 1; break;  /* hex output */
   default: usage(0); return 2;
  }
 }
 if (optind == argc) {usage(0); return 2; }
 else if (flags.mode == list) {
  for (int i=optind; i<argc; i++) {
   int attrLen = listxattr(argv[i], NULL, 0, 0);
   if (attrLen == 0) continue;  /* Print message? */
   else if (attrLen == -1) {
    fprintf(stderr, "xattr: %s: ", argv[i]); perror(NULL); continue;
   }
   char* attrList = malloc(attrLen);
   checkMem(attrList);
   attrLen = listxattr(argv[i], attrList, attrLen, 0);
   if (attrLen == 0) {free(attrList); continue; }
   else if (attrLen == -1) {
    /* Check for errno == ERANGE? */
    free(attrList);
    fprintf(stderr, "xattr: %s: ", argv[i]);
    perror(NULL);
    continue;
   }
   printf("%s:\n", argv[i]);
   char* currAttr = attrList;
   while (attrLen > 0) {
    printf(" %s", currAttr);
    if (flags.v) {
     printf(": ");
     int valLen = getxattr(argv[i], currAttr, NULL, 0, 0, 0);
     if (valLen == -1) {
      fprintf(stderr, "\nxattr: %s: %s: ", argv[i], currAttr); perror(NULL);
      continue;
     }
     char* value = malloc(valLen);
     checkMem(value);
     valLen = getxattr(argv[i], currAttr, value, valLen, 0, 0);
     if (valLen == -1) {
      /* Check for errno == ERANGE? */
      free(value);
      fprintf(stderr, "\nxattr: %s: %s: ", argv[i], currAttr); perror(NULL);
      continue;
     }
     if (flags.x) {
      for (int j=0; j<valLen; j++) {
       if (j>0) putchar(' '); printf("%02X", (unsigned char) value[j]);
      }
     } else {fwrite(value, 1, valLen, stdout); }
     free(value);
    }
    putchar('\n');
    char* nextAttr = strchr(currAttr, '\0');
    if (nextAttr == NULL) break;
    attrLen -= ++nextAttr - currAttr;
    currAttr = nextAttr;
   }
   free(attrList);
  }
 }






void usage(_Bool verbose) {

}
