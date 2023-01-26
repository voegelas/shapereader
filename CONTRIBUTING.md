# Contributing

The code for this C library is hosted at
https://github.com/voegelas/shapereader.

Grab the latest version using the command:

    git clone https://github.com/voegelas/shapereader.git

You can submit code changes by forking the repository, pushing your code
changes to your clone, and then submitting a pull request.

Please use clang-format and the included .clang-format file to format your
code.

Do only use functions from the C standard library and POSIX.  Do not use
functions from <math.h> that require the mathematical library libm.

The library is managed with autoconf, automake, make and pkg-config.  Once
installed, here are some commands you might try:

    autoreconf -i
    ./configure
    make
    make check VERBOSE=1
