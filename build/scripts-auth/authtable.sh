#!/bin/bash

# Author: Ted Kirkpatrick
# Last edited: 2016-03-29
#
# This Bash script returns the contents of table AuthTable and displays
# them using authfields.py.
#
# To execute, run "./authtable.sh"
#
# Variables:
#   $B is the localhost portion of the URI for the basic server, defined
#     in setvars.h.

curl --silent -X get $B/ReadEntityAdmin/AuthTable | ./authfields.py
