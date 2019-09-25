# Building Static Binary
The build script doesn't completely work currently. Following the docker command in build.sh you can start a docker container and get it to work manually. ```docker run -it --rm -v \`pwd\`/io:/io ubuntu:trusty /bin/bash``` 

Inside the container if you run `bash /io/compile.sh` it will run until failing to compile libvips because of an error about not being able to find a Makefile for the /po subproject. Remove that directory from the root Makefile, run `make` manually and libvips will compile properly. You then can run the remaining commands in /io/compile.sh manually to create the tifig binary. Don't forget to source the /io/env.sh file to get the environment variables.
