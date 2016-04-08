#!/usr/bin/python2

# Author: Ted Kirkpatrick
# Last edited: 2016-03-28
#
# This Python 2 program reads a json object and prints the value of the object
# named "token".

import json

line = raw_input()
obj = json.loads(line)
print obj["token"]
