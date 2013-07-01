#!/bin/bash

SERVEREXEC=$1
PLUGINEXEC=$2
VERBOSE=$3
TESTS=${*:4}

run_tests()
{
	COUNT=0
	SUCCESS=0
	
	for i in $TESTS ; do 
		COUNT=$((COUNT+1))
		$SERVEREXEC $PLUGINEXEC "$i" $VERBOSE; 
		
		if [ "$?" == "0" ] ; then
			SUCCESS=$((SUCCESS+1))
		fi
	done
	
	echo "Done: $SUCCESS of $COUNT tests passed."
}

run_tests;
