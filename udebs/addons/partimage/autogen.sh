#!/bin/sh

echo "---> Running aclocal..."
aclocal -I macros
echo "---> Running autoheader..."
autoheader
echo "---> Running automake..."
automake --add-missing --copy
echo "---> Running autoconf..."
autoconf

echo "Done."
