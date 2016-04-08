#!/bin/bash

# Author: Ted Kirkpatrick
# Last edited: 2016-03-29
#
# This Bash script takes 4 parameters (row name (representing the username),
# password, DataPartition value, and DataRow value) and creates (or updates)
# the user with the given username in table AuthTable, partition Userid.
#
# To execute, run "./adduser.sh username password datapartition datarow"
#
# Variables:
#   $2 is the password.
#   $3 is the DataPartition value.
#   $4 is the DataRow value.
#   $H is the declaration of a JSON type, defined in setvars.h.
#   $B is the localhost portion of the URI for the basic server, defined
#     in setvars.h.
#   $1 is the row name (representing the username).

if [ $# -eq 4 ]
then
  PWD=\"Password\"\:\"$2\"
  PA=\"DataPartition\"\:\"$3\"
  PR=\"DataRow\"\:\"$4\"
  curl -iX put -H "$H" -d "{$PWD, $PA, $PR}" $B/UpdateEntityAdmin/AuthTable/Userid/$1

else
  echo "Usage: ./adduser.sh username password datapartition datarow"

fi
