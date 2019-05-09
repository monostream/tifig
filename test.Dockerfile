FROM alpine

RUN echo '@edge http://dl-cdn.alpinelinux.org/alpine/edge/testing' >> /etc/apk/repositories

RUN apk --no-cache add \
    vips-dev@edge \
    ffmpeg-dev \
    fftw-dev \
    g++ \
    cmake \
    make

RUN apk --no-cache add \
    bash \
    py2-pip \
    py-pillow \
    py-scipy \
    py-argparse \
    python-dev

RUN pip install numpy --upgrade
RUN pip install pyssim colorprint

ADD . /tifig

RUN \
    mkdir /tifig/build &&\
    cd /tifig/build &&\
    cmake .. &&\
    make