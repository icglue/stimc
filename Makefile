#
#   stimc is a lightweight verilog-vpi wrapper for stimuli generation.
#   Copyright (C) 2019-2022  Andreas Dixius, Felix Neumärker
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

DOXYDIR          = doc

DOXYFILE         = doxy/lib.doxyfile

BROWSER         ?= firefox

ICPRO_DIR        = $(CURDIR)/examples
REGDIR           = $(ICPRO_DIR)/regression

BUILD_BASE       = lib
BUILD_LIB_SO     = lib/libstimc.so
BUILD_LIB_A      = lib/libstimc.a
BUILD_HEADERS    = src/*.h
BUILD_PCFILES    = pkgconfig/*.pc

VERSION         ?= 1.2

LOCTOOL         ?= cloc
LOCSOURCES       = $(wildcard lib/src/*.c lib/src/*.c++ lib/src/*.cpp lib/src/*.h)

PREFIX          ?= /usr/local
DESTDIR         ?=
LIBDIR          ?= $(PREFIX)/lib
PCDIR           ?= $(PREFIX)/lib/pkgconfig
INCLUDEDIR      ?= $(PREFIX)/include
SRCDIR          ?= $(PREFIX)/src
DOCDIR          ?= $(PREFIX)/share/doc
MANDIR          ?= $(PREFIX)/share/man

THREAD_IMPL     ?= libcolocal

PACK_FORMAT     ?= tar.gz
PACK_NAME       ?= stimc
PACK_PREFIX     ?= $(PACK_NAME)/
PACK_REF        ?= HEAD

#-------------------------------------------------------
# LIB
.PHONY: lib libco libbuild

ifeq ($(THREAD_IMPL),libcolocal)
libbuild: libco
endif

lib: libbuild

libbuild:
	@$(MAKE) THREAD_IMPL=$(THREAD_IMPL) --no-print-directory -C $(BUILD_BASE)
	@$(MAKE) THREAD_IMPL=$(THREAD_IMPL) --no-print-directory -C $(BUILD_BASE) pkgconfig

libco:
	@$(MAKE) --no-print-directory -C libco

.DEFAULT_GOAL: lib

#-------------------------------------------------------
# Install
INSTALL_LIBDIR     = $(DESTDIR)$(LIBDIR)
INSTALL_INCLUDEDIR = $(DESTDIR)$(INCLUDEDIR)
INSTALL_SRCDIR     = $(DESTDIR)$(SRCDIR)
INSTALL_PCDIR      = $(DESTDIR)$(PCDIR)
INSTALL_DOCDIR     = $(DESTDIR)$(DOCDIR)
INSTALL_MANDIR     = $(DESTDIR)$(MANDIR)

install: lib
	install -m 644 -D -t $(INSTALL_INCLUDEDIR) $(wildcard $(addprefix $(BUILD_BASE)/, $(BUILD_HEADERS)))
	install -m 644 -D -t $(INSTALL_PCDIR)      $(wildcard $(addprefix $(BUILD_BASE)/, $(BUILD_PCFILES)))
	install -m 755 -D -T $(addprefix $(BUILD_BASE)/, $(BUILD_LIB_A))  $(INSTALL_LIBDIR)/$(notdir $(BUILD_LIB_A))
	install -m 755 -D -T $(addprefix $(BUILD_BASE)/, $(BUILD_LIB_SO)) $(INSTALL_LIBDIR)/$(notdir $(BUILD_LIB_SO)).$(VERSION)
	ldconfig -n $(INSTALL_LIBDIR)
	ln -fs $(notdir $(BUILD_LIB_SO)).$(VERSION) $(INSTALL_LIBDIR)/$(notdir $(BUILD_LIB_SO))
	install -m 644 -D -t $(INSTALL_DOCDIR)/stimc README.md

install-man: docs
	install -m 644 -D -t $(INSTALL_MANDIR)/man3 $(wildcard $(addprefix $(DOXYDIR)/man/man3/, stimc*))
	mv $(INSTALL_MANDIR)/man3/stimc__.h.3 $(INSTALL_MANDIR)/man3/stimc++.h.3
	for f in $(INSTALL_MANDIR)/man3/* ; do gzip -f $$f ; done

install-addons:
	install -m 644 -D -t $(INSTALL_SRCDIR)/stimc-addons $(wildcard $(addprefix $(BUILD_BASE)/addons/, *.c *.cpp *.h))

install-all: install install-man install-addons

.PHONY: install-all install install-man install-addons

#-------------------------------------------------------
# documentation
.PHONY: showdocs doclib docs

doclib: $(DOXYFILE) | $(DOXYDIR)
	-doxygen $(DOXYFILE)

docs: doclib

showdocs:
	$(BROWSER) $(DOXYDIR)/html/index.html > /dev/null 2> /dev/null &

#-------------------------------------------------------
# LoC
.PHONY: loc locall

loc:
	@$(LOCTOOL) $(LOCSOURCES)

locall:
	@$(LOCTOOL) $(LOCSOURCES) $(LOCEXTRA) $(LOCTEMPLATES)

#-------------------------------------------------------
# Test
.PHONY: test simcheck memcheck cleantest

export ICPRO_DIR
cleantest:
	@$(MAKE) --no-print-directory -sC $(REGDIR) cleanall

test: lib
	@$(MAKE) --no-print-directory simcheck
	@$(MAKE) --no-print-directory memcheck

simcheck: cleantest
	@echo "=== running simulations ==="
	@$(MAKE) --no-print-directory -sC $(REGDIR)
	@$(MAKE) --no-print-directory -sC $(REGDIR) show | ./scripts/regression_eval.sh
	@echo ""

memcheck: cleantest
	@echo "=== running memchecks ==="
	@$(MAKE) --no-print-directory -sC $(REGDIR) memcheck
	@echo ""

#-------------------------------------------------------
# version-updaet
updateversion:
	@./scripts/update_version.sh $(VERSION)

.PHONY: updateversion

#-------------------------------------------------------
# directories
$(DOXYDIR):
	mkdir -p $@

#-------------------------------------------------------
# cleanup targets
.PHONY: mrproper cleanall cleandoc cleanlib

cleanlib:
	@$(MAKE) --no-print-directory -C $(BUILD_BASE) clean

cleanlibco:
	@$(MAKE) --no-print-directory -C libco clean

cleandoc:
	@rm -rf $(DOXYDIR)

mrproper cleanall: cleandoc cleantest cleanlib cleanlibco

#-------------------------------------------------------
# archive
.PHONY: pack

PACK_OUTPUT = $(PACK_NAME).$(PACK_FORMAT)
PACK_FLAGS  = --prefix=$(PACK_PREFIX) $(PACK_REF)

pack: $(PACK_OUTPUT)

$(PACK_NAME).tar.zst:
	@git archive --format=tar $(PACK_FLAGS) | zstd -10 - -o $@
$(PACK_NAME).tar.xz:
	@git archive --format=tar $(PACK_FLAGS) | xz > $@
$(PACK_NAME).tar.gz:
	@git archive --format=tar $(PACK_FLAGS) | gzip > $@
$(PACK_NAME).tar.bz2:
	@git archive --format=tar $(PACK_FLAGS) | bzip2 > $@
$(PACK_NAME).zip:
	@git archive --format=zip $(PACK_FLAGS) > $@
