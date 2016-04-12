# Author: Jeffrey Leung
# Last edited: 2016-03-28
#
# This script sets shell variable TR to contain an authentication token
# from Microsoft Azure.

TR=$(./getreadtoken.sh username password | ./parsetoken.py)
export TR
echo "To use the token, call the shell variable \$TR"
