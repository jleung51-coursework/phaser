#!/bin/sh

# Author: Ted Kirkpatrick
# Last edited: 2016-03-28
#
# This sh script takes 2 parameters (userid, password) and returns an
# authentication token from Microsoft Azure for the credentials.
#
# To execute, run "./getreadtoken.sh userid password"
#
# Variables:
#   $H is the declaration of a JSON type, defined in setvars.h.
#   $2 is the password.
#   $A is the localhost portion of the URI for the authentication server,
#     defined in setvars.h.
#   $1 is the userid.

if [ $# -eq 2 ] ; then
  curl -X get --silent -H "$H" -d "{\"Password\": \"$2\"}" $A/GetReadToken/$1
else
  echo "usage: ./getreadtoken.sh userid password"
fi
