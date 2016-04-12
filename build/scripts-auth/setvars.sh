# Author: Ted Kirkpatrick
# Last edited: 2016-03-28
#
# This sh script declares 3 variables for easier use of curl commands
# to access tables in Microsoft Azure.
#
# To execute, run "source setvars.sh"
#
# Variables:
#   $A is the localhost portion of the URI for the authentication server,
#     defined in setvars.h.
#   $B is the localhost portion of the URI for the basic server,
#     defined in setvars.h.
#   $H is the declaration of a json type, defined in setvars.h.

A='http://localhost:34570'
export A
B='http://localhost:34568'
export B
H='Content-type: application/json'
export H
