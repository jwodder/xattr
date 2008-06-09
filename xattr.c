/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fnmatch.h>
#include <sys/xattr.h>

#define checkMem(p)  if (!p) {perror("xattr"); exit(1); }

struct {_Bool v, x, A, i : 1; enum {list=0, set, rm} mode; int slink; } flags;

void usage(_Bool verbose);
char** getAttrs(char* path);
char* getValue(char* path, char* attr, int* valLen);

int main(int argc, char** argv) {
 int ch;
 while ((ch = getopt(argc, argv, "lsvhxrPLAi")) != -1) {
  switch (ch) {
   case 'l': flags.mode = list; break;
   case 's': flags.mode = set; break;
   case 'v': flags.v = 1; break;
   case 'h': usage(1); return 0;
   case 'x': flags.x = 1; break;
   case 'r': flags.mode = rm; break;
   case 'L': flags.slink = 0; break;  /* Follow (all) symbolic links */
   case 'P': flags.slink = XATTR_NOFOLLOW; break;  /* Follow no symlinks */
   case 'A': flags.A = 1; break;
   case 'i': flags.i = 1; break;  /* Do case-insensitive pattern matching */
   default: usage(0); return 2;
  }
 }
 if (optind == argc) {usage(0); return 2; }
 if (flags.mode == list) {
  if (flags.A) {
   for (int i=optind; i<argc; i++) {
    char** attrs = getAttrs(argv[i]);
    if (attrs == NULL) continue;  /* Print message? */
    printf("%s:\n", argv[i]);
    for (int j=0; attrs[j] != NULL; j++) {
     printf(" %s", attrs[j]);
     if (flags.v) {
      printf(" : ");
      int valLen;
      char* value = getValue(argv[i], attrs[j], &valLen);
      if (value == NULL) continue;
      if (flags.x) {
       for (int j=0; j<valLen; j++) {
	if (j>0) putchar(' '); printf("%02X", (unsigned char) value[j]);
       }
      } else {fwrite(value, 1, valLen, stdout); }
      free(value);
     }
     putchar('\n');
    }
    if (attrs[0]) free(attrs[0]);
    free(attrs);
    putchar('\n');
   }
  } else {
   if (optind == argc-1) {usage(0); return 2; }
   char** attrs = getAttrs(argv[argc-1]);
   if (attrs == NULL) return 0;
   for (int i=optind; i<argc-1; i++) {
    for (int j=0; attrs[j] != NULL; j++) {
     if (fnmatch(argv[i], attrs[j], FNM_PERIOD | (flags.i ? FNM_CASEFOLD : 0)) == 0) {
      printf("%s", attrs[j]);
      if (flags.v) {
       printf(" : ");
       int valLen;
       char* value = getValue(argv[argc-1], attrs[j], &valLen);
       if (value == NULL) continue;
       if (flags.x) {
	for (int j=0; j<valLen; j++) {
	 if (j>0) putchar(' '); printf("%02X", (unsigned char) value[j]);
	}
       } else {fwrite(value, 1, valLen, stdout); }
       free(value);
      }
      putchar('\n');
     }
    }
   }
   if (attrs[0]) free(attrs[0]);
   free(attrs);
  }
 } else if (flags.mode == set) {
  if (optind == argc-1) {usage(0); return 2; }
  for (int i=optind; i<argc-1; i++) {
   char* pair = strdup(argv[i]);
   /* Parse backslashes while looking for the equals sign */
   /* Although an error occurs when setxattr() is passed an empty name, don't
    * try to stop the user from putting an equals sign at the beginning of an
    * argument. */
   int escOffset = 0;
   char* newVal = NULL;
   for (int j=0; pair[j] != '\0'; j++) {
    if (pair[j] == '\\') {j++; escOffset++; }
    else if (pair[j] == '=') {
     pair[j-escOffset] = '\0';
     newVal = pair + j + 1;
     break;
    }
    if (escOffset > 0) pair[j-escOffset] = pair[j];
   }
   if (newVal == NULL) {
    free(pair);
    fprintf(stderr, "xattr: %s: invalid argument\n", argv[i]); continue;
   }
   if (flags.x) {
    char* value = malloc(strlen(newVal)/2 + 1);
    checkMem(value);
    int hexOff=0, byteOff=0, scanRet, bytesRead;
    while ((scanRet = sscanf(newVal + hexOff, " %2hhx%n", value+byteOff,
     &bytesRead)) != EOF) {
     if (scanRet == 0) {
      if (flags.v) fprintf(stderr, "xattr: warning: parsing of `%s' value "
       "terminated at character %d: not hexadecimal pair\n", argv[i], hexOff);
      break;
     }
     hexOff += bytesRead;
     byteOff++;
    }
    if (setxattr(argv[argc-1], pair, value, byteOff, 0, flags.slink) == -1) {
     fprintf(stderr, "xattr: %s: %s: ", argv[argc-1], pair); perror(NULL);
    } else if (flags.v)
     printf("xattr: attribute `%s' set on %s\n", pair, argv[argc-1]);
    free(value);
   } else {
    if (setxattr(argv[argc-1], pair, newVal, strlen(newVal), 0, flags.slink) ==
     -1) {
     fprintf(stderr, "xattr: %s: %s: ", argv[argc-1], pair); perror(NULL);
    } else if (flags.v)
     printf("xattr: attribute `%s' set on %s\n", pair, argv[argc-1]);
   }
   free(pair);
  }
 } else if (flags.mode == rm) {
  if (flags.A) {
   for (int i=optind; i<argc; i++) {
    char** attrs = getAttrs(argv[i]);
    if (attrs == NULL) continue;
    for (int j=0; attrs[j] != NULL; j++) {
     if (removexattr(argv[i], attrs[j], flags.slink) == -1) {
      fprintf(stderr, "xattr: %s: %s: ", argv[i], attrs[j]); perror(NULL);
     } else if (flags.v) {
      printf("xattr: attribute `%s' removed from %s\n", attrs[j], argv[i]);
     }
    }
    if (attrs[0]) free(attrs[0]);
    free(attrs);
   }
  } else {
   char* file = argv[argc-1];
   if (optind == argc-1) {usage(0); return 2; }
   char** attrs = getAttrs(file);
   if (attrs == NULL) return 0;
   for (int i=optind; i<argc-1; i++) {
    for (int j=0; attrs[j] != NULL; j++) {
     if (fnmatch(argv[i], attrs[j], FNM_PERIOD | (flags.i ? FNM_CASEFOLD : 0)) == 0) {
      if (removexattr(file, attrs[j], flags.slink) == -1) {
       fprintf(stderr, "xattr: %s: %s: ", file, attrs[j]); perror(NULL);
      } else if (flags.v) {
       printf("xattr: attribute `%s' removed from %s\n", attrs[j], file);
      }
     }
    }
   }
   if (attrs[0]) free(attrs[0]);
   free(attrs);
  }
 }
 return 0;
}

void usage(_Bool verbose) {
 fprintf(stderr, "Usage: xattr [-l] [-LPvx] pattern [pattern ...] file\n"
  "       xattr [-l] -A [-LPvx] file [file ...]\n"
  "       xattr -s [-LPvx] name=value [name=value ...] file\n"
  "       xattr -r [-LPv] pattern [pattern ...] file\n"
  "       xattr -r -A [-LPv] file [file ...]\n"
  "       xattr -h\n"
  "`pattern' is a shell wildcard pattern.\n");
 if (verbose)
  fprintf(stderr, "Options:\n"
   "  -A - list or remove all attributes\n"
   "  -h - display this help message & exit\n"
   "  -i - perform case-insensitive pattern matching\n"
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

char** getAttrs(char* path) {
 if (!path) return NULL;  /* Just in case */
 int attrLen = listxattr(path, NULL, 0, flags.slink);
 if (attrLen == 0) return NULL;
 else if (attrLen == -1) {
  fprintf(stderr, "xattr: %s: ", path); perror(NULL); return NULL;
 }
 char* attrList = malloc(attrLen);
 checkMem(attrList);
 attrLen = listxattr(path, attrList, attrLen, flags.slink);
 if (attrLen == 0) {free(attrList); return NULL; }
 else if (attrLen == -1) {
  /* Check for errno == ERANGE? */
  free(attrList);
  fprintf(stderr, "xattr: %s: ", path); perror(NULL);
  return NULL;
 }
 char* currAttr = attrList;
 int currLen = attrLen, attrQty = 0;
 while (currLen > 0) {
  attrQty++;
  char* nextAttr = strchr(currAttr, '\0');
  if (nextAttr == NULL) break;
  currLen -= ++nextAttr - currAttr;
  currAttr = nextAttr;
 }
 char** attrs = calloc(attrQty+1, sizeof(char*));
 checkMem(attrs);
 currAttr = attrList;
 for (int i=0; i<attrQty; i++) {
  attrs[i] = currAttr;
  char* nextAttr = strchr(currAttr, '\0');
  if (nextAttr == NULL) break;
  currAttr = ++nextAttr;
 }
 attrs[attrQty] = NULL;  /* Just to be sure */
 return attrs;
}

char* getValue(char* path, char* attr, int* valLen) {
 *valLen = getxattr(path, attr, NULL, 0, 0, flags.slink);
 if (*valLen == -1) {
  fprintf(stderr, "\nxattr: %s: %s: ", path, attr); perror(NULL); return NULL;
 }
 char* value = malloc(*valLen);
 checkMem(value);
 *valLen = getxattr(path, attr, value, *valLen, 0, flags.slink);
 if (*valLen == -1) {
  /* Check for errno == ERANGE? */
  free(value);
  fprintf(stderr, "\nxattr: %s: %s: ", path, attr); perror(NULL);
  return NULL;
 }
 return value;
}
