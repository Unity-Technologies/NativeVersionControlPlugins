#!/usr/bin/python
#
# Asset state int to text representation
#

import sys

stateMap = { 
    "None" : 0,
    "Local" : 1 << 0,
    "Synced" : 1 << 1,
    "OutOfSync" : 1 << 2,
    "Missing" : 1 << 3,
    "CheckedOutLocal" : 1 << 4,
    "CheckedOutRemote" : 1 << 5,
    "DeletedLocal" : 1 << 6,
    "DeletedRemote" : 1 << 7,
    "AddedLocal" : 1 << 8,
    "AddedRemote" : 1 << 9,
    "Conflicted" : 1 << 10,
    "LockedLocal" : 1 << 11,
    "LockedRemote" : 1 << 12,
    "Updating" : 1 << 13,
    "ReadOnly" : 1 << 14,
    "MetaFile" : 1 << 15
    }

stateArr = stateMap.items()
stateArr.sort(key=lambda x: x[1])
stateArr.reverse()

state = int(sys.argv[1])

while state != 0:
    for i in stateArr:
        if i[1] <= state:
            print i[0] + " ",
            state = state - i[1]
            break;

