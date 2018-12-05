#!/bin/bash -eu

PATH=/tifig/build:$PATH

function convert_file() {
  file=$1
  out=$(dirname $file)/$(basename ${file%.*}).jpg
  echo "converting $file to $out"
  tifig -v -p $file $out
  touch -r $file $out
}

function convert_directory() {
  docker run -v `pwd`/convert.sh:/root/convert.sh -v $1:/images -it tifig:latest /root/convert.sh -D /images
}

while [ -n "$*" ]; do
  case "$1" in
    -d)
      convert_directory $2
      shift; shift
      ;;
    -D)
      shift
      for f in `find $1 -iname "*.heic"`; do
        convert_file $f
      done
      shift; shift
      ;;
    *)
      convert_directory $1
      shift
  esac
done
