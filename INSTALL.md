NSnake INSTALL
==============

To build nsnake, you need a C compiler and ncurses.

Note for Windows
----------------

NSnake works on Windows but it does not have a native curses library, however
you can use [pdcurses][] which is the implementation known to work with NSnake.

Build
-----

Go to the source directory and type the following commands:

    make
    make install

Build configuration
-------------------

The following options may be used to configure the build.

- `GID`: change the gid to chown (default: games),
- `UID`: change the uid to chown (default: games),
- `PREFIX`: root directory for installation
- `BINDIR`: change to the installation of executable (default: PREFIX/bin),
- `MANDIR`: change to the manual page location (default: PREFIX/share/man),
- `VARDIR`: change the score file database directory (default: PREFIX/var).

Also, edit config.mk file to adjust NSnake to your system otherwise fallback
implementations will be bundled in.

Scores file
-----------

NSnake uses a scores file in order to share all users scores on the same
machine. In order to work, the binary `nsnake` but have setgid file attribute
and its database directory with the appropriate permissions. This is
automatically done as `make install` step.

The directory for saving the scores is writable by this group to make sure the
executable can write it from any user.

[pdcurses]: https://pdcurses.org
