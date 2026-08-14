#!/bin/bash

# build the library and run the test suite (from the build directory)

cmake -DCMAKE_BUILD_TYPE=Debug .. && make
if [ $? != 0 ]
then
    echo "make test failed!"
    exit 1
fi

if [ "$1" != "" ]
then
    ctest --output-on-failure -R "^$1-"
else
    ctest --output-on-failure
fi
