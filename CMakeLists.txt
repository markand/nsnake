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

cmake_minimum_required(VERSION 3.0)
project(nsnake C)

option(WITH_DOCS "Install documentation" On)
option(WITH_MAN "Install manuals" On)

set(WITH_DOCDIR share/doc/irccd CACHE STRING "Directory for additional documentation")
set(WITH_MANDIR share/man CACHE STRING "Directory for manuals")
set(WITH_DBDIR var/db/nsnake CACHE STRING "Score directory")

set(WITH_USER games CACHE STRING "Binary uid")
set(WITH_GROUP games CACHE STRING "Binary gid")

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)

check_include_file(err.h HAVE_ERR_H)

check_function_exists(err HAVE_ERR)
check_function_exists(errx HAVE_ERRX)
check_function_exists(resizeterm HAVE_RESIZETERM)
check_function_exists(random HAVE_RANDOM)
check_function_exists(srandom HAVE_SRANDOM)

check_symbol_exists(SIGWINCH curses.h HAVE_SIGWINCH)

if (NOT HAVE_ERR_H OR NOT HAVE_ERRX)
	set(HAVE_ERR FALSE)
endif ()

if (NOT HAVE_SRANDOM)
	set(HAVE_RANDOM FALSE)
endif ()

if (MSVC)
	message(FATAL_ERROR "NSnake support MinGW only")
endif ()

if (IS_ABSOLUTE ${WITH_DBDIR})
	set(NSNAKE_SCORESDIR ${WITH_DBDIR})
else ()
	set(NSNAKE_SCORESDIR ${CMAKE_INSTALL_PREFIX}/${WITH_DBDIR})
endif ()

configure_file(
	${nsnake_SOURCE_DIR}/cmake/config.h.in
	${nsnake_BINARY_DIR}/config.h
)

add_executable(nsnake nsnake.c)

#
# For Windows, libpdcurses is bundled in nsnake, for other platforms, search for the curses library.
#
# We also install MinGW runtimes to the binary directory.
#
if (WIN32)
	if (CMAKE_SIZEOF_VOID_P MATCHES 8)
		set(ARCH amd64)
	else ()
		set(ARCH x86)
	endif ()

	set(INCLUDES ${nsnake_SOURCE_DIR}/windows/${ARCH}/libpdcurses/include)
	set(LIBS ${nsnake_SOURCE_DIR}/windows/${ARCH}/libpdcurses/lib/libpdcurses.a)
	install(FILES ${nsnake_SOURCE_DIR}/windows/${ARCH}/libwinpthread-1.dll DESTINATION bin)
else ()
	find_package(Curses REQUIRED)
	set(INCLUDES ${CURSES_INCLUDE_DIRS})
	set(LIBS ${CURSES_LIBRARIES})
endif ()

target_link_libraries(nsnake ${LIBS})
target_include_directories(nsnake PRIVATE ${INCLUDES} ${nsnake_BINARY_DIR})

#
# Installation of files.
#
install(TARGETS nsnake DESTINATION bin)

#
# Post install script for database directory and permissions.
#
configure_file(
	${nsnake_SOURCE_DIR}/cmake/post-install.cmake.in
	${nsnake_BINARY_DIR}/post-install.cmake
	@ONLY
)

install(SCRIPT ${nsnake_BINARY_DIR}/post-install.cmake)

if (WITH_MAN)
	install(FILES nsnake.6 DESTINATION ${WITH_MANDIR}/man6)
endif ()

if (WITH_DOCS)
	install(FILES LICENSE.md DESTINATION ${WITH_DOCDIR})
endif ()
