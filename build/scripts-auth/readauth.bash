#!/bin/bash

# Author: Jeffrey Leung
# Last edited: 2016-03-28
#
# This Bash script returns the contents of table AuthTable.
#
# Variables:
#   $B is the localhost portion of the URI, defined in setvars.sh.
#   $TR is the authentication token from Microsoft Azure, defined manually
#   by the user.

if [ $# -eq 0 ]
then
  curl -iX get $B/ReadEntityAuth/AuthTable/$TR

else
  echo "Usage: ./readauth.bash"

fi
