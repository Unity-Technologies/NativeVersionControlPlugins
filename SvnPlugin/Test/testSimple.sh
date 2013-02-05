#!/bin/sh
TPWD=$(pwd)
PLUGIN=../../SubversionPlugin


echo "c:pluginConfig pluginTraits" | $PLUGIN
echo "c:pluginConfig pluginVersions 1 2 3 4 5" | $PLUGIN
echo "c:pluginConfig svnRepos http://test.com/fish" | $PLUGIN
echo "c:pluginConfig svnUsername theUser" | $PLUGIN
echo "c:pluginConfig svnPassword thePassword" | $PLUGIN
echo "c:pluginConfig svnOptions theOptions" | $PLUGIN
echo "c:add Hello adding" | $PLUGIN

statusTests()
{

echo "------------------------------------- status A"
$TPWD/$PLUGIN <<EOF
c:status
1
Assets/NewBehaviourScript.cs
42
EOF

echo "------------------------------------- status B"
$TPWD/$PLUGIN <<EOF
c:status
1
.
42
EOF

echo "------------------------------------- changes"
$TPWD/$PLUGIN <<EOF
c:changes
EOF

echo "------------------------------------- incoming"
$TPWD/$PLUGIN <<EOF
c:incoming
EOF

echo "------------------------------------- incomingAssets"
$TPWD/$PLUGIN <<EOF
c:incomingChangeAssets
1
EOF

echo "------------------------------------- incomingAssets"
$TPWD/$PLUGIN <<EOF
c:incomingChangeAssets
2
EOF

echo "------------------------------------- changeStatus"
$TPWD/$PLUGIN <<EOF
c:changeStatus
testcl2
EOF

echo "------------------------------------- changeStatus"
$TPWD/$PLUGIN <<EOF
c:changeStatus
-1
EOF

echo "// Fish it good\n" >> Assets/NewBehaviourScript.cs

echo "------------------------------------- changeStatus"
$TPWD/$PLUGIN <<EOF
c:changeStatus
-1
EOF

echo "------------------------------------- revert"
$TPWD/$PLUGIN <<EOF
c:revert 
1
$PWD/Assets/NewBehaviourScript.cs
0
EOF

echo "------------------------------------- changeStatus"
$TPWD/$PLUGIN <<EOF
c:changeStatus
-1
EOF

echo "// Fish it good for change\n" >> Assets/NewBehaviourScript.cs

echo "------------------------------------- changeStatus"
$TPWD/$PLUGIN <<EOF
c:changeStatus
-1
EOF

echo "------------------------------------- submit"
$TPWD/$PLUGIN <<EOF
c:pluginConfig assetsPath $PWD/Assets
c:submit 
-1
This is a test desciption\nAnd this is the second line
1
$PWD/Assets/NewBehaviourScript.cs
0
EOF

echo "------------------------------------- changeStatus"
$TPWD/$PLUGIN <<EOF
c:changeStatus
-1
EOF

return 0;
}

pushd ~/UnityProjects/SubversionTest/Repos1/ && statusTests 
popd
