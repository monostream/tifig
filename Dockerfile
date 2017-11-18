FROM guetux/alpine_vips_ffmpeg
MAINTAINER Stefan Reinhard, Flavian Sierk

ADD . /tifig
RUN \
	mkdir /tifig/build && \
    cd /tifig/build && \
    cmake .. && \
    make
