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

#
# General setup.
# -------------------------------------------------------------------
#

set(NSNAKE_VERSION_MAJOR 2)
set(NSNAKE_VERSION_MINOR 0)
set(NSNAKE_VERSION_PATCH 0)
set(NSNAKE_VERSION "${NSNAKE_VERSION_MAJOR}.${NSNAKE_VERSION_MINOR}.${NSNAKE_VERSION_PATCH}")

set(CMAKE_C_FLAGS "-Wall -Wextra ${CMAKE_CXX_FLAGS}")

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)

#
# Check for games:games.
# -------------------------------------------------------------------
#
# On non-Windows platforms, determine if the uid and gid games is available, if
# it's not we use nobody instead.
#

if (NOT WIN32)
	include(cmake/check-games.cmake)
endif ()

if (HAVE_USER_GAMES MATCHES 0)
	set(DEFAULT_USER games)
else ()
	set(DEFAULT_USER nobody)
endif ()

if (HAVE_GROUP_GAMES MATCHES 0)
	set(DEFAULT_GROUP games)
else ()
	set(DEFAULT_GROUP nobody)
endif ()

if (WIN32)
	set(DEFAULT_MAN Off)
else ()
	set(DEFAULT_MAN On)
endif ()

#
# The curses library
# -------------------------------------------------------------------
#
if (WIN32)
	if (CMAKE_SIZEOF_VOID_P MATCHES 8)
		set(ARCH amd64)
	else ()
		set(ARCH x86)
	endif ()

	set(INCLUDES ${nsnake_SOURCE_DIR}/windows/${ARCH}/libpdcurses/include)
	set(LIBS ${nsnake_SOURCE_DIR}/windows/${ARCH}/libpdcurses/lib/libpdcurses.a)
else ()
	find_package(Curses REQUIRED)
	set(INCLUDES ${CURSES_INCLUDE_DIRS})
	set(LIBS ${CURSES_LIBRARIES})
endif ()

#
# Options.
# -------------------------------------------------------------------
#

option(WITH_DOCS "Install documentation" On)
option(WITH_MAN "Install manuals" ${DEFAULT_MAN})

set(WITH_DOCDIR share/doc/irccd CACHE STRING "Directory for additional documentation")
set(WITH_MANDIR share/man CACHE STRING "Directory for manuals")
set(WITH_DBDIR var/db/nsnake CACHE STRING "Score directory")
set(WITH_USER ${DEFAULT_USER} CACHE STRING "Binary uid")
set(WITH_GROUP ${DEFAULT_GROUP} CACHE STRING "Binary gid")

#
# Platform checks.
# -------------------------------------------------------------------
#

set(CMAKE_REQUIRED_LIBRARIES ${LIBS})

check_include_file(err.h HAVE_ERR_H)

check_function_exists(err HAVE_ERR)
check_function_exists(errx HAVE_ERRX)
check_function_exists(resizeterm HAVE_RESIZETERM)
check_function_exists(resize_term HAVE_RESIZE_TERM)
check_function_exists(random HAVE_RANDOM)
check_function_exists(srandom HAVE_SRANDOM)

check_symbol_exists(SIGWINCH signal.h HAVE_SIGWINCH)

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

if (IS_ABSOLUTE ${WITH_MANDIR})
	set(NSNAKE_MANDIR ${WITH_MANDIR})
else ()
	set(NSNAKE_MANDIR ${CMAKE_INSTALL_PREFIX}/${WITH_MANDIR})
endif ()

if (IS_ABSOLUTE ${WITH_DOCDIR})
	set(NSNAKE_DOCDIR ${WITH_DOCDIR})
else ()
	set(NSNAKE_DOCDIR ${CMAKE_INSTALL_PREFIX}/${WITH_DOCDIR})
endif ()

configure_file(
	${nsnake_SOURCE_DIR}/cmake/config.h.in
	${nsnake_BINARY_DIR}/config.h
)

#
# Main nsnake executable.
# -------------------------------------------------------------------
#

add_executable(nsnake nsnake.c)

target_link_libraries(nsnake ${LIBS})
target_include_directories(nsnake PRIVATE ${INCLUDES} ${nsnake_BINARY_DIR})

#
# Installation stuff.
# -------------------------------------------------------------------
#

configure_file(
	${nsnake_SOURCE_DIR}/cmake/post-install.cmake.in
	${nsnake_BINARY_DIR}/post-install.cmake
	@ONLY
)

install(TARGETS nsnake DESTINATION bin)
install(SCRIPT ${nsnake_BINARY_DIR}/post-install.cmake)

if (WITH_MAN)
	install(FILES nsnake.6 DESTINATION ${WITH_MANDIR}/man6)
endif ()

if (WITH_DOCS)
	install(FILES LICENSE.md DESTINATION ${WITH_DOCDIR})
endif ()

if (WIN32)
	install(
		FILES ${nsnake_SOURCE_DIR}/windows/${ARCH}/libwinpthread-1.dll
		DESTINATION bin
	)
endif ()

#
# Packaging stuff.
# -------------------------------------------------------------------
#

set(CPACK_PACKAGE_VERSION ${NSNAKE_VERSION})
set(CPACK_SOURCE_PACKAGE_FILE_NAME "nsnake-${NSNAKE_VERSION}")
set(CPACK_SOURCE_GENERATOR ZIP TXZ)
set(CPACK_SOURCE_IGNORE_FILES .hg .hgignore)
include(CPack)

#
# Summary.
# -------------------------------------------------------------------
#

message("Compiling nsnake with the following flags:")
message("    General flags:    ${CMAKE_C_FLAGS}")
message("    Debug flags:      ${CMAKE_C_FLAGS_DEBUG}")
message("    Release flags:    ${CMAKE_C_FLAGS_RELEASE}")
message("")

message("Building nsnake with the following options:")

if (NOT WIN32)
	message("    User (uid):       ${WITH_USER}")
	message("    Groupe (gid):     ${WITH_GROUP}")
endif ()

if (WITH_MAN)
	set(WITH_MAN_MSG "Yes")
else ()
	set(WITH_MAN_MSG "No")
endif ()

if (WITH_DOCS)
	set(WITH_DOCS_MSG "Yes")
else ()
	set(WITH_DOCS_MSG "No")
endif ()

message("    Installing man:   ${WITH_MAN_MSG}")
message("    Installing docs:  ${WITH_DOCS_MSG}")
message("")

message("Installing nsnake in the following directories:")
message("    Documentation:    ${NSNAKE_DOCDIR}")
message("    Manuals:          ${NSNAKE_MANDIR}")
message("    Scores directory: ${NSNAKE_SCORESDIR}")
message("")
