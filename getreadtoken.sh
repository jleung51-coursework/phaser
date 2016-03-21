#!/bin/sh
if [ $# -eq 2 ] ; then
  curl -X get -H 'Content-type: application/json' -d "{\"Password\": \"$2\"}" http://localhost:34570/GetReadToken/$1
else
  echo "usage: getreadtoken userid password"
fi
