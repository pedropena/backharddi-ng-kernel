#! /bin/sh -e
# lndir - create shadow link tree
#
# Originally from X.
# Improved by Jeff Bailey <jbailey@ubuntu.com>.
#
# Used to create a copy of the a directory tree that has links for all
# non-directories (except those named RCS, SCCS or CVS.adm).  If you are
# building the distribution on more than one machine, you should use
# this technique.
#
# If your master sources are located in /usr/local/src/X and you would like
# your link tree to be in /usr/local/src/new-X, do the following:
#
# 	%  mkdir /usr/local/src/new-X
#	%  cd /usr/local/src/new-X
# 	%  lndir ../X

USAGE="Usage: $0 fromdir [todir]"

if [ $# -lt 1 -o $# -gt 2 ]; then
    echo "$USAGE"
    exit 1
fi

if [ $# -ne 2 ]; then
    DIRTO="."
else
    DIRTO="$2"
    if [ -e $DIRTO ]; then
	echo "$0: Target directory exists."
	exit 2
    fi
    mkdir -p "$DIRTO"
fi

DIRTO="$(readlink -f $DIRTO)"
DIRFROM="$(readlink -f $1)"

if [ ! -d "$DIRFROM" ]; then
    echo "$0: $DIRFROM is not a directory"
    echo "$USAGE"
    exit 2
fi

if [ "$DIRFROM" = "$DIRTO" ]; then
    echo "$0: $DIRFROM: FROM and TO are identical!"
    exit 1
fi

# parse output of "ls" below more carefully
IFS='
'
for file in $(ls -af $DIRFROM); do
    [ "$DIRFROM/$file" != "$DIRTO" ] || continue
    if [ -d "$DIRFROM/$file" -a ! -L "$DIRFROM/$file" ]; then
	case "$file" in
	    .|..|RCS|SCCS|CVS.adm) continue ;;
	esac
	sh $0 "$DIRFROM/$file" "$DIRTO/$file" 
    else
	ln -s "$DIRFROM/$file" "$DIRTO"
    fi
done

