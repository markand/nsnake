#
# CMakeLists.txt -- CMake build system for nsnake
#
# Copyright (c) 2011-2016 David Demelier <markand@malikania.fr>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

#
# Check if 'games' uid/gid is available using getent.
#

execute_process(
    COMMAND getent passwd games
    RESULT_VARIABLE HAVE_USER_GAMES
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND getent group games
    RESULT_VARIABLE HAVE_GROUP_GAMES
    OUTPUT_QUIET
    ERROR_QUIET
)

if (HAVE_USER_GAMES MATCHES 0)
    set(HAVE_USER_GAMES TRUE)
else ()
    set(HAVE_USER_GAMES FALSE)
endif ()

if (HAVE_GROUP_GAMES MATCHES 0)
    set(HAVE_GROUP_GAMES TRUE)
else ()
    set(HAVE_GROUP_GAMES FALSE)
endif ()
