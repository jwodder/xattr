all : xattr xattr.1

xattr : xattr.c
	gcc -o xattr xattr.c -std=c99 -O2

xattr.1 : xattr.pod
	pod2man -c '' -r '' xattr.pod xattr.1
