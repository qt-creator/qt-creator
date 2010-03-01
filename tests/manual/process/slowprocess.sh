#!/bin/sh

# Emulate a slow process with continous output

I=0
STDERR=0
if [ " $1" = " -e" ]
then
  echo stderr
  STDERR=1
  shift 1  
fi

while [ $I -lt 20 ]
do
  if [ $STDERR -ne 0 ]
  then
    echo $I 1>&2
  else 
    echo $I
  fi
  I=`expr $I + 1`
  sleep 1
done
