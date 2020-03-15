#
#   stimc is a lightweight verilog-vpi wrapper for stimuli generation.
#   Copyright (C) 2017-2020  Andreas Dixius, Felix Neumärker
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

DOCDIR                = doc

DOXYFILE              = doxy/lib.doxyfile

BROWSER              ?= firefox

LOCTOOL              ?= cloc
LOCSOURCES            = $(wildcard lib/src/*.c lib/src/*.c++ lib/src/*.h)


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
# directories
$(DOCDIR):
	mkdir -p $@

#-------------------------------------------------------
# cleanup targets
.PHONY: mrproper cleanall cleandoc

cleandoc:
	rm -rf $(DOCDIR)

mrproper cleanall: cleandoc

