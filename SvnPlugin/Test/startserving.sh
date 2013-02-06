#!/bin/bash
#
# Start serving a repos root
#

ROOT=$1
if [ "x$ROOT" == "x" ] ;
then
	echo "Usage: $0 <repository root>"
	exit 1
fi

svnserve -d --pid-file=$ROOT/svnserve.pid -r $ROOT
