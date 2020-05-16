#
#   stimc is a lightweight verilog-vpi wrapper for stimuli generation.
#   Copyright (C) 2019-2020  Andreas Dixius, Felix Neumärker
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

DOCDIR           = doc

DOXYFILE         = doxy/lib.doxyfile

BROWSER         ?= firefox

ICPRO_DIR        = $(CURDIR)/examples
REGDIR           = $(ICPRO_DIR)/regression

BUILD_BASE       = lib
BUILD_LIB_SO     = lib/libstimc.so
BUILD_LIB_A      = lib/libstimc.a
BUILD_HEADERS    = src/*.h
BUILD_PCFILES    = pkgconfig/*.pc

VERSION         ?= 0.1

LOCTOOL         ?= cloc
LOCSOURCES       = $(wildcard lib/src/*.c lib/src/*.c++ lib/src/*.cpp lib/src/*.h)

PREFIX          ?= /usr/local
DESTDIR         ?=
LIBDIR          ?= $(PREFIX)/lib
PCDIR           ?= $(PREFIX)/lib/pkgconfig
INCLUDEDIR      ?= $(PREFIX)/include

#-------------------------------------------------------
# LIB
.PHONY: lib

lib:
	@$(MAKE) --no-print-directory -C $(BUILD_BASE)
	@$(MAKE) --no-print-directory -C $(BUILD_BASE) pkgconfig

.DEFAULT_GOAL: lib

#-------------------------------------------------------
# Install
INSTALL_LIBDIR     = $(DESTDIR)$(LIBDIR)
INSTALL_INCLUDEDIR = $(DESTDIR)$(INCLUDEDIR)
INSTALL_PCDIR      = $(DESTDIR)$(PCDIR)

install: lib
	install -m 644 -D -t $(INSTALL_INCLUDEDIR) $(wildcard $(addprefix $(BUILD_BASE)/, $(BUILD_HEADERS)))
	install -m 644 -D -t $(INSTALL_PCDIR)      $(wildcard $(addprefix $(BUILD_BASE)/, $(BUILD_PCFILES)))
	install -m 755 -D -T $(addprefix $(BUILD_BASE)/, $(BUILD_LIB_A))  $(INSTALL_LIBDIR)/$(notdir $(BUILD_LIB_A))
	install -m 755 -D -T $(addprefix $(BUILD_BASE)/, $(BUILD_LIB_SO)) $(INSTALL_LIBDIR)/$(notdir $(BUILD_LIB_SO)).$(VERSION)
	ldconfig -n $(INSTALL_LIBDIR)
	ln -fs $(notdir $(BUILD_LIB_SO)).$(VERSION) $(INSTALL_LIBDIR)/$(notdir $(BUILD_LIB_SO))


#-------------------------------------------------------
# documentation
.PHONY: showdocs doclib docs

doclib: $(DOXYFILE) | $(DOCDIR)
	-doxygen $(DOXYFILE)

docs: doclib

showdocs:
	$(BROWSER) $(DOCDIR)/html/index.html > /dev/null 2> /dev/null &

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

test:
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
# directories
$(DOCDIR):
	mkdir -p $@

#-------------------------------------------------------
# cleanup targets
.PHONY: mrproper cleanall cleandoc cleanlib

cleanlib:
	@$(MAKE) --no-print-directory -C $(BUILD_BASE) clean

cleandoc:
	@rm -rf $(DOCDIR)

mrproper cleanall: cleandoc cleantest cleanlib

