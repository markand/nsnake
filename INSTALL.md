NSnake INSTALL
==============

Installation instructions.

Requirements
------------

- POSIX system, known to work under:
  - Linux
  - FreeBSD, NetBSD, OpenBSD, DragonFlyBSD
  - Windows using cygwin
- C11 compiler, tested with:
  - GCC 9, Clang 9
- ncurses library.
- GNU make.

Build
-----

Go to the source directory and type the following commands:

	make
	make install

On some systems you make need to use `gmake` rather than make.

Build configuration
-------------------

The following options may be used to configure the build.

- `GID`: change the gid to chown (default: root),
- `UID`: change the uid to chown (default: games),
- `PREFIX`: root directory for installation
- `BINDIR`: change to the installation of executable (default: PREFIX/bin),
- `MANDIR`: change to the manual page location (default: PREFIX/share/man),
- `VARDIR`: change the score file database directory (default: PREFIX/var).

Scores file
-----------

NSnake uses a scores file in order to share all users scores on the same
machine. In order to work, your user must have read/write permissions to the
directory which is created at install time, it defaults to UID:GID (see above).

The directory for saving the scores is writable by this group to make sure your
user can write it from any user.
