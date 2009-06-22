#!/bin/sh
#
# $Id: autogen.sh,v 1.11 2009/06/22 15:23:40 couannette Exp $
#

# Font directory
# default:
fontsdir=/usr/X11/lib/X11/fonts/TTF

if [ -f /etc/debian_version ] ; then
    if [ -d /usr/share/fonts/truetype/ttf-bitstream-vera ] ; then
	fontsdir=/usr/share/fonts/truetype/ttf-bitstream-vera
	echo "Fonts dir: $fontsdir"
    else
	echo "Debian system: please install the ttf-bitstream-vera font package."
    fi
else
    echo "Default fonts dir: $fontsdir"
fi

# Target
target=motif
case $(uname -s) in
    Darwin) 
	echo "Mac system: default target is x11"
	echo "(Carbon is not yet supported by this build system)"
	echo "Add this to the configure command line:"
	echo "--with-target=x11"
	echo
	target=x11

	port=$(port version)
	if [ $? -eq 0 ] ; then
	    # we have Mac Ports installed
	    add_path=/opt/local
	fi
	;;
esac

if [ ! -z "$add_path" ] ; then
    cflags="CPPFLAGS=-I$add_path/include"
    lflags="LDFLAGS=-L$add_path/lib"
fi

my_options="--with-fontsdir=$fontsdir --with-target=$target $cflags $lflags $*"

echo
echo "configure options: $my_options"
echo

autoreconf --force --install && \
./configure $my_options
