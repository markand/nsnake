NSnake CHANGES
==============

nsnake current
-----------------------

- Switch back to plain Makefiles,
- Removing bundled libraries,
- Do almost all platform detection in nsnake.h.

nsnake 2.0.0 2016-02-26
-----------------------

- Switch to CMake,
- Port to Windows,
- Add -v option for verbosity.

nsnake 1.4.0 2012-01-12
-----------------------

- Prevent food for spawning on snake,
- Added a cute animation when quitting,
- Switch to stdbool instead of int,
- Remove useless function parameters.

nsnake 1.3.0 2011-08-28
-----------------------

- Fix a security when reading a bad (or unexistant) file,
- Do not append the same score twice,
- Fix a failure when a food is near the wall and the snake wallcrosses,
- Added an option to disable scoring.

nsnake 1.2.1 2011-08-26
-----------------------

- Pause is working again.

nsnake 1.2.0 2011-08-25
-----------------------

- Added option to set the color,
- Switch to binary score file,
- Reduced snake's length to uint16\_t from size\_t.

nsnake 1.1.0 2011-04-03
-----------------------

- Bug where food spawn on snake fixed,
- New feature : if screen is filled, snake is reduced to the default size,
- Show scores according to -w switch,
- Improved security parsing nicknames in the score file,
- Added more security in insertscore() function,
- Improved the pause state, now possible to quit,
- Better showscore() output.

nsnake 1.0.1 2011-03-08
-----------------------

- Little fix in the Makefile for the FreeBSD port.

nsnake 1.0.0 2011-03-08
-----------------------

- Initial release.
