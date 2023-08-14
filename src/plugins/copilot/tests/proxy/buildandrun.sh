#!/bin/bash

docker build --build-arg PWDMODE=with -t copilot-proxy-test . && \
docker run --rm -it -p 3128:3128 copilot-proxy-test
