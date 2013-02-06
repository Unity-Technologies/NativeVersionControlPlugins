#!/bin/bash
#
# Run all tests
#

TEST_PATH=`pwd`

mkdir -p Results

REPO_PATH=/var/folders/h8/v3xp3vps5xlgm0jyw3fzv0d40000gp/T/svntestRoot.rB5dlFZL
# REPO_PATH=`./setup.sh | sed -ne 's/^Repo[[:space:]]*: \(.*\)$/\1/ p'`

killall svnserve 2>/dev/null >/dev/null

ensure_sever_running()
{
	if ! ps aux | grep "svnserve -d --pid-file" | grep -v "grep" >/dev/null ; then
		pushd $TEST_PATH >/dev/null
		./startserving.sh $REPO_PATH 
		popd >/dev/null
	fi
}

ensure_sever_running;

pushd $REPO_PATH/COA >/dev/null
svn --username testuser --password testpassword switch --relocate file://$REPO_PATH/ROOT svn://localhost/ROOT

TMP=`mktemp -t svndifftmp`

for I in $TEST_PATH/Tests/*Test ; do
	N=`basename $I`
	echo -n "Test $N ";
	ensure_sever_running;
	cat $I | $TEST_PATH/../../SubversionPlugin > $TEST_PATH/Results/$N
	diff -Naur $TEST_PATH/Baseline/$N $TEST_PATH/Results/$N > $TMP
	if [ "$?" != "0" ] ; then
		echo "FAILED"
		lam -s "    " $TMP
		exit 1
	else
		echo "OK"
	fi
done

popd >/dev/null

killall svnserve 2>/dev/null >/dev/null
