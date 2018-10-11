#!/bin/sh
# Run this to generate all the initial makefiles, etc.

# shellcheck disable=SC2006
srcdir=`dirname "$0"`
test -z "$srcdir" && srcdir=.

PKG_NAME="gutenprint"

(test -f "$srcdir/configure.ac") || {
    echo "**Error**: Directory "\`$srcdir\'" does not look like the top-level directory"
    exit 1
}

if test $# -gt 0 ; then
    case "$1" in
	-h|--help|--help=*)
	if test -f "$srcdir/configure" ; then
	    exec "$srcdir/configure" "$1"
	fi ;;
	*) ;;
    esac
fi

# shellcheck source=./scripts/autogen.sh
. $srcdir/scripts/autogen.sh
