#!/bin/bash

icons="c cpp ui pri pro qml" 

for icon in $icons; 
do
  convert ${icon}16.png ${icon}32.png ${icon}48.png  ${icon}64.png ${icon}128.png ${icon}.ico 
done
