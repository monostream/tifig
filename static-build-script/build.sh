#!/bin/bash

docker run -i --rm -v `pwd`/io:/io \
  ubuntu:trusty \
  bash /io/compile.sh