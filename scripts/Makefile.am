## Process this file with automake to produce Makefile.in
## Copyright (C) 2000 Roger Leigh
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <https://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in.

@SET_MAKE@

include $(top_srcdir)/scripts/global.mk


## Variables

# The autotools-installed scripts /should/ be automatically distributed.
# build-release needs to be in the tarball because it's run during
# make distcheck, although it's not intended to be used outside
# of the development sandbox
EXTRA_DIST = \
	autogen.sh \
	build-release \
	count-cpus \
	global.mk \
	gversion.in \
	snapstamp \
	test-gversion.in \
	mkgitlog.in \
	wzpaq.in

noinst_SCRIPTS= \
	build-release \
	gversion \
	snapstamp \
	test-gversion \
	mkgitlog \
	wzpaq

## Clean

MAINTAINERCLEANFILES = \
	Makefile.in \
	compile \
	config.guess \
	config.rpath \
	config.sub \
	depcomp \
	gversion \
	install-sh \
	ltconfig \
	ltmain.sh \
	mdate-sh \
	missing \
	mkgitlog \
	mkinstalldirs \
	test-driver \
	test-gversion \
	wzpaq \
	ylwrap
