#!/usr/bin/python3

# Author: Ted Kirkpatrick
# Last edited: 2016-03-29
#
# This Python 3 program reads a JSON array consisting of the entities in
# a given table and prints the entries one per line, sorted by user ID and
# with fields in a specified order.

import json

# Return sort key for AuthTable entries
def key (a):
    return (a['Partition'], a['Row'])

# Return a list of non-required fields in an AuthTable entry
def other_fields(e):
    result = []
    for fn in e:
        if fn not in {'Partition', 'Row', 'DataPartition', 'DataRow', 'Password'}:
            result.append((fn,e[fn]))
    return result

# Return an AuthTable entry as a tuple with its fields in a specified order
def entry_to_list(e):
    return [e['Row'], e['Password'], e['DataPartition'], e['DataRow']] + other_fields(e)

# Main routine
array = json.loads(input ())
array.sort(key=key)

for obj in array:
    print (entry_to_list(obj))
