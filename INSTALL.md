NSnake INSTALL
==============

To build nsnake, you need a C compiler, [CMake][] and the curses library.

Note for Windows
----------------

NSnake has been ported to Windows using pdcurses, it is bundled with nsnake and does not need to be installed
separately.

Build
-----

Go to the source directory and type the following commands

````
mkdir _build_
cd _build_
cmake .. -DCMAKE_BUILD_TYPE=Release
make
make install
````

See the CMake documentation for other generators.

Options
-------

The following boolean options are available:

  - **WITH_DOCS**: set to false if you don't want documentation to be installed,
  - **WITH_MAN**: set to false if you don't want manual pages to be installed.

The following directories may be adjusted:

  - **WITH_DOCDIR**: the directory where to install documentation files,
  - **WITH_MANDIR**: the root directory where to install manual files.

Example:

````
cmake .. -DWITH_DOCS=Off -DWITH_MANDIR=man
````

[CMake]: http://cmake.org