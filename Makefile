xattr : xattr.c
	gcc -o xattr xattr.c -std=c99 -O2

xattr-db : xattr.c
	gcc -o xattr-db xattr.c -std=c99 -g -Wall
