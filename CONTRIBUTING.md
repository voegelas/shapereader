# Contributing

The code for this C library is hosted at
https://github.com/voegelas/shapereader.

Grab the latest version using the command:

    git clone https://github.com/voegelas/shapereader.git

You can submit code changes by forking the repository, pushing your code
changes to your clone, and then submitting a pull request.

Please use clang-format and the included .clang-format file to format your
code.

Use only functions from the C standard library.  You can use POSIX functions in
the tests.

The library is managed with either CMake or the GNU Autotools and pkg-config.

## CMake

    mkdir build
    cd build
    cmake ..
    make
    make test

## GNU Autotools

    autoreconf -i
    mkdir build
    cd build
    ../configure
    make
    make check VERBOSE=1
