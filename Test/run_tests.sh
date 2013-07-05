#!/bin/bash

SERVEREXEC=$1
PLUGINEXEC=$2
VERBOSE=$3
TESTS=${*:4}
PLUGINNAME=`basename $PLUGINEXEC`
BASELINEDIR=Test/NewBaseline/$PLUGINNAME

# Create the dir where log files are stored
mkdir Library 2>/dev/null
rm -rf $BASELINEDIR 2>&1 > /dev/null
mkdir -p $BASELINEDIR

run_tests()
{
	COUNT=0
	SUCCESS=0
	
	for i in $TESTS ; do 
		COUNT=$((COUNT+1))
		NAME=`basename $i` ;
		NEWBASELINE=$BASELINEDIR/$NAME ;
		if [ "$VERBOSE" == "newbaseline" ] ; then
			$SERVEREXEC $PLUGINEXEC "$i" $VERBOSE > "$NEWBASELINE" ;
			echo "diff -Naur $i $NEWBASELINE" ;
			diff -Naur "$i" "$NEWBASELINE" ;
		else
			$SERVEREXEC $PLUGINEXEC "$i" $VERBOSE;
		fi

		if [ "$?" == "0" ] ; then
			SUCCESS=$((SUCCESS+1))
		fi
	done
	
	echo "Done: $SUCCESS of $COUNT tests passed."
}

run_tests;
