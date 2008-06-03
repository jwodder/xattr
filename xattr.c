/* $Id$ */

/* To do:
 - Implement:
  - printing of attribute size?
  - switch to only list certain attributes?
 - Take care of single hex digits in hex attribute values
 - Modify the return value if a non-fatal error occurred during execution?
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/xattr.h>

#define checkMem(p)  if (!p) {perror("xattr"); exit(1); }

struct {_Bool v, x : 1; enum {list=0, set, rm} mode; int slink; } flags;

void usage(_Bool verbose);

int main(int argc, char** argv) {
 int ch;
 while ((ch = getopt(argc, argv, "lsvhxrPL")) != -1) {
  switch (ch) {
   case 'l': flags.mode = list; break;
   case 's': flags.mode = set; break;
   case 'v': flags.v = 1; break;
   case 'h': usage(1); return 0;
   case 'x': flags.x = 1; break;
   case 'r': flags.mode = rm; break;
   case 'L': flags.slink = 0; break;  /* Follow (all) symbolic links */
   case 'P': flags.slink = XATTR_NOFOLLOW; break;  /* Follow no symlinks */
   default: usage(0); return 2;
  }
 }
 if (optind == argc) {usage(0); return 2; }
 else if (flags.mode == list) {
  for (int i=optind; i<argc; i++) {
   int attrLen = listxattr(argv[i], NULL, 0, flags.slink);
   if (attrLen == 0) continue;  /* Print message? */
   else if (attrLen == -1) {
    fprintf(stderr, "xattr: %s: ", argv[i]); perror(NULL); continue;
   }
   char* attrList = malloc(attrLen);
   checkMem(attrList);
   attrLen = listxattr(argv[i], attrList, attrLen, flags.slink);
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
     printf(" : ");
     int valLen = getxattr(argv[i], currAttr, NULL, 0, 0, flags.slink);
     if (valLen == -1) {
      fprintf(stderr, "\nxattr: %s: %s: ", argv[i], currAttr); perror(NULL);
      continue;
     }
     char* value = malloc(valLen);
     checkMem(value);
     valLen = getxattr(argv[i], currAttr, value, valLen, 0, flags.slink);
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
 } else if (flags.mode == set) {
  if ((argc - optind) % 2 == 0) {
   fprintf(stderr, "xattr: invalid number of options for -s switch\n\n");
   usage(0); return 2;
  }
  char* file = argv[argc-1];
  for (int i=optind; i<argc-1; i+=2) {
   if (flags.x) {
    char* value = malloc(strlen(argv[i+1])/2 + 1);
    checkMem(value);
    int hexOff=0, byteOff=0, scanRet, bytesRead;
    while ((scanRet = sscanf(argv[i+1] + hexOff, " %2hhx%n", value+byteOff,
     &bytesRead)) != EOF) {
     if (scanRet == 0) {
      if (flags.v) fprintf(stderr, "xattr: warning: parsing of `%s' value "
       "terminated at character %d: not hexadecimal pair\n", argv[i], hexOff);
      break;
     }
     hexOff += bytesRead;
     byteOff++;
    }
    if (setxattr(file, argv[i], value, byteOff, 0, flags.slink) == -1) {
     fprintf(stderr, "xattr: %s: %s: ", file, argv[i]); perror(NULL);
    } else if (flags.v)
     printf("xattr: Attribute `%s' set on %s\n", argv[i], file);
    free(value);
   } else {
    if (setxattr(file, argv[i], argv[i+1], strlen(argv[i+1]), 0, flags.slink) == -1) {
     fprintf(stderr, "xattr: %s: %s: ", file, argv[i]); perror(NULL);
    } else if (flags.v)
     printf("xattr: Attribute `%s' set on %s\n", argv[i], file);
   }
  }
 } else if (flags.mode == rm) {
  char* file = argv[argc-1];
  for (int i=optind; i<argc-1; i++) {
   if (removexattr(file, argv[i], flags.slink) == -1) {
    fprintf(stderr, "xattr: %s: %s: ", file, argv[1]); perror(NULL);
   } else if (flags.v) {
    printf("xattr: attribute `%s' removed from %s\n", argv[i], file);
   }
  }
 }
 return 0;
}

void usage(_Bool verbose) {
 fprintf(stderr, "Usage: xattr [-l] [-LPvx] file ...\n"
  "       xattr -s [-LPvx] name value [name value ...] file\n"
  "       xattr -r [-LPv] name [name ...] file\n"
  "       xattr -h\n");
 if (verbose)
  fprintf(stderr, "Options:\n"
   "  -h - display this help message & exit\n"
   "  -L - follow symbolic links\n"
   "  -l - list extended attribute names (default)\n"
   "  -P - do not follow symbolic links (default)\n"
   "  -r - remove extended attributes\n"
   "  -s - set extended attribute values\n"
   "  -v - verbose output\n"
   "  -x - output/read attribute values as hexadecimal\n"
  );
 else fprintf(stderr, "Run `xattr -h' for a command-line options summary.\n");
}
