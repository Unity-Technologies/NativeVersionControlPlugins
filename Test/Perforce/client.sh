#!/bin/bash
P4EXEC=$1
P4SERVER=$2
P4CLIENT=$3
P4ROOT=$(pwd)/Test/tmp/$P4CLIENT

setup_workspace()
{
	echo "Setting up workspace $P4CLIENT in $P4ROOT"
	mkdir -p $P4ROOT
	$P4EXEC -H $P4SERVER client -i  <<EOF

Client:$P4CLIENT

Update:2013/02/19 09:13:18

Access:2013/06/24 12:38:18

Owner:$USER

Description:
    Created by $USER.

Root:$P4ROOT

Options:noallwrite noclobber nocompress unlocked nomodtime normdir

SubmitOptions:submitunchanged

LineEnd:local

View:
    //depot/... //$P4CLIENT/...
EOF

}

teardown_workspace()
{
	echo "Tearing down workspace $WORKSPACE"
	$P4EXEC -H $P4SERVER client -d $P4CLIENT
}


if [ "$4" == "setup" ]; then 
	setup_workspace; 
elif [ "$4" == "teardown" ]; then 
	teardown_workspace; 
fi
