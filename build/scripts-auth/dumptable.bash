#!/bin/bash

# Author: Jeffrey Leung
# Last edited: 2016-03-28
#
# This Bash script returns the contents of a requested table.
# If no parameters are given, then the table will default to "DataTable".
# If 1 parameter is given, then the table will return the contents of the
# table named as the parameter.
#
# Variables:
#   $B is the localhost portion of the URI, defined in setvars.sh.
#   $1 is the table name, and is the optional first parameter.
#   $TR is the authentication token from Microsoft Azure,
#   defined manually by the user.

if [ $# -eq 0 ]
then
  TABLE=DataTable
  curl -iX get $B/ReadEntityAuth/$TABLE/$TR

elif [ $# -eq 1 ]
then
  TABLE=$1
  curl -iX get $B/ReadEntityAuth/$TABLE/$TR

else
  echo "Usage: ./dumptable.bash [table]"
  echo "Table defaults to \"DataTable\" if no parameter is given"

fi
