#!/bin/sh

docker build -t qt-5-ubuntu-20.04-build -f Dockerfile-qt-5-ubuntu-20.04-build .
docker build -t qt-5-ubuntu-20.04-run -f Dockerfile-qt-5-ubuntu-20.04-run .
