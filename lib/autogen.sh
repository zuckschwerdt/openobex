#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

DIE=0

# Check for autoconf, the required version is set in configure.in
(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have at minimum autoconf version 2.12 installed"
	echo "to compile OBEX. Download the appropriate package for"
	echo "your distribution, or get the source tarball at"
	echo "ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
}

# Check for libtool
(libtool --version | egrep "1.2") > /dev/null || {
	(libtool --version | egrep "1.3") > /dev/null || {
		echo
		echo "You must have at minimum libtool version 1.2 installed"
		echo "to compile OBEX. Download the appropriate package for"
		echo "your distribution, or get the source tarball at"
		echo "ftp://alpha.gnu.org/gnu/libtool-1.2d.tar.gz"
		DIE=1
	}
}

# Check for automake, the required version is set in Makefile.am
(automake --version) < /dev/null > /dev/null 2>&1 ||{
	echo
	echo "You must have at minimum automake version 1.3 installed"
	echo "to compile OBEX. Download the appropriate package for"
	echo "your distribution, or get the source tarball at"
	echo "ftp://ftp.cygnus.com/pub/home/tromey/automake-1.3b.tar.gz"
	DIE=1
}


if test "$DIE" -eq 1; then
	exit 1
fi

(test -f src/obex_main.c) || {
	echo "You must run this script in the top-level OBEX directory"
	exit 1
}

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

case $CC in
xlc )
  am_opt=--include-deps;;
esac

for i in .
do 
  echo processing $i
  (cd $i; \
    libtoolize --copy --force; \
    aclocal $ACLOCAL_FLAGS;
    if test "$i" != "libIDL"; then autoheader; fi; \
    automake --add-missing $am_opt; \
    if test "$i" != "libIDL"; then autoheader; fi; \
    autoconf)
done

cd $ORIGDIR

echo "Running $srcdir/configure --enable-maintainer-mode" "$@"
$srcdir/configure --enable-maintainer-mode "$@"

echo 
echo "Now type 'make' to compile OBEX."
