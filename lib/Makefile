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

# ===============================================================================
#
#  Description: Common makefile for applications and libraries
#
#===============================================================================

#-------------  name of the executable binary  ---------------------------------
EXECUTABLE      ?=
LIB             ?= libstimc.so
STATICLIB       ?= libstimc.a

VERSION         ?= 0.1
VERSION_MAIN     = $(shell echo $(VERSION) | sed 's/\([0-9]\+\)\..*/\1/')
SONAME           = $(LIB).$(VERSION_MAIN)

#-------------  name of the target (can also be a library *.{a,so})  -----------
TARGET          ?= $(LIB) $(STATICLIB)

#-------------  directory of the target  ---------------------------------------
TARGET_DIR      ?= ./lib

#-------------  list of all source files  --------------------------------------
SOURCES         ?= *.c *.cpp *.c++ *.cc

#-------------  directory of source files --------------------------------------
SOURCE_DIR      ?= src

#-------------  pkg-config for includes and libs  ------------------------------
PKG_CFG_LIBS    ?=

#-------------  directories of includes  ---------------------------------------
INC_DIR         ?= src

#-------------  directories of libraries  --------------------------------------
LIB_DIR         ?=

#-------------  library linkflags  ---------------------------------------------
LIBS            ?=

#-------------  thread backend  ------------------------------------------------
# can be one of pcl, libco, boost1, boost2
THREAD_IMPL     ?= pcl

#-------------  pkg-config settings  -------------------------------------------
PREFIX          ?= /usr/local
LIBDIR          ?= $(PREFIX)/lib
INCLUDEDIR      ?= $(PREFIX)/include
PC_SRC           = ./pkgconfig/stimc.pc.src
PC_TRG           = ./pkgconfig/stimc.pc

#-------------  directory of the dependency  -----------------------------------
DEPS_DIR        ?= ./.deps

#-------------  directory of the asm files -------------------------------------
CREATE_ASM      ?= NO

#-------------  directory of the asm files -------------------------------------
ASM_DIR         ?= ./.asm

#-------------  use linktime optimization  -------------------------------------
USE_LTO         ?= NO

# ------------  compiler flags  ------------------------------------------------
WARNFLAGS       ?= -Wall -Wextra -Wshadow -pedantic
OPTFLAGS        ?= -O2 -fdata-sections -ffunction-sections
ARCHFLAGS       ?=
CFLAGS          ?= -std=c11
CXXFLAGS        ?= -std=c++11 -Weffc++
CPPFLAGS        := $(addprefix -D, $(DEFS) $(PREDEFS)) $(addprefix -I, $(INC_DIR))
LDFLAGS         ?= -Wl,-O1 -Wl,--gc-sections
ASMFLAG         ?= -adhln

# ------------  build-tools  ---------------------------------------------------
GP              ?=
CC              ?= $(GP)gcc
CXX             ?= $(GP)g++
LD              ?= $(GP)ld
OD              ?= $(GP)objdump
STRIP           ?= $(GP)strip
AR              ?= $(GP)ar
LN              ?= ln -sf
RM              ?= rm -f
RMDIR           ?= rmdir
MKDIR           ?= mkdir -p

ifeq ($(USE_LTO), YES)
  OPTFLAGS      := -flto $(OPTFLAGS)
  LDFLAGS       := -flto -fuse-linker-plugin $(LDFLAGS)
endif

ifeq ($(suffix $(LIB)), .so)
  CFLAGS        := -fpic $(CFLAGS)
  CXXFLAGS      := -fpic $(CXXFLAGS)
  LDFLAGS       := -shared $(LDFLAGS) -Wl,-soname=$(SONAME)
endif

# stimc thread implementation backend
ifeq ($(THREAD_IMPL),pcl)
CPPFLAGS        += -D STIMC_THREAD_IMPL_PCL
THREAD_CPPFLAGS +=
THREAD_LDFLAGS  += -lpcl
endif
ifeq ($(THREAD_IMPL),libco)
CPPFLAGS        += -D STIMC_THREAD_IMPL_LIBCO
PKG_CFG_LIBS    += libco
endif
ifeq ($(THREAD_IMPL),boost2)
CPPFLAGS        += -D STIMC_THREAD_IMPL_BOOST2
THREAD_CPPFLAGS +=
THREAD_LDFLAGS  += -lboost_context
endif
ifeq ($(THREAD_IMPL),boost1)
CPPFLAGS        += -D STIMC_THREAD_IMPL_BOOST1
THREAD_CPPFLAGS +=
THREAD_LDFLAGS  += -lboost_coroutine
endif

#pkg-config
ifneq ($(PKG_CFG_LIBS),)
  CPPFLAGS      += $(shell pkg-config --cflags $(PKG_CFG_LIBS))
  LDFLAGS       += $(shell pkg-config --libs $(PKG_CFG_LIBS))
endif

# vpi
VPI_CPPFLAGS    ?= $(shell iverilog-vpi --cflags | egrep -o -- '-I\s*\S*')
VPI_LDFLAGS     ?= $(shell iverilog-vpi --ldflags | egrep -o -- '-[lL]\s*\S*')

ALL_CFLAGS      := $(WARNFLAGS) $(OPTFLAGS) $(ARCHFLAGS) $(CFLAGS) $(CPPFLAGS) $(VPI_CPPFLAGS) $(THREAD_CPPFLAGS)
ALL_CXXFLAGS    := $(WARNFLAGS) $(OPTFLAGS) $(ARCHFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(VPI_CPPFLAGS) $(THREAD_CPPFLAGS)
ALL_LDFLAGS     := $(LDFLAGS) $(VPI_LDFLAGS) $(THREAD_LDFLAGS)

# ------------  generate the names of the source/object files  -----------------
ALL_SOURCES      = $(wildcard $(addprefix $(SOURCE_DIR)/,$(SOURCES)))
BASENAMES        = $(basename $(ALL_SOURCES:$(SOURCE_DIR)/%=%))
OBJECTS          = $(addprefix $(TARGET_DIR)/, $(addsuffix .o, $(BASENAMES)))
DEPS             = $(addprefix $(DEPS_DIR)/, $(addsuffix .d, $(BASENAMES)))

# ------------ basic compile rule ----------------------------------------------

.DEFAULT_GOAL := all

all: $(addprefix $(TARGET_DIR)/, $(TARGET))

$(DEPS_DIR):
	@$(MKDIR) $(DEPS_DIR)

$(TARGET_DIR):
	@$(MKDIR) $(TARGET_DIR)

$(ASM_DIR):
ifeq ($(CREATE_ASM), YES)
	@$(MKDIR) $(ASM_DIR)
endif

ifeq ($(CREATE_ASM), YES)
  GEN_ASM_FLAGS = -Wa,$(ASMFLAG)=$(ASM_DIR)/$*.s
else
  GEN_ASM_FLAGS =
endif


$(DEPS_DIR)/%.d: $(SOURCE_DIR)/%.c Makefile | $(DEPS_DIR)
	@$(CC) -MM -E $(CPPFLAGS) $(VPI_CPPFLAGS) $< | perl -p -e 's#[^:]*:#$(TARGET_DIR)/$$&#' > $@

$(DEPS_DIR)/%.d: $(SOURCE_DIR)/%.cc Makefile | $(DEPS_DIR)
	@$(CXX) -MM -E $(CPPFLAGS) $(VPI_CPPFLAGS) $< | perl -p -e 's#[^:]*:#$(TARGET_DIR)/$$&#' > $@
$(DEPS_DIR)/%.d: $(SOURCE_DIR)/%.c++ Makefile | $(DEPS_DIR)
	@$(CXX) -MM -E $(CPPFLAGS) $(VPI_CPPFLAGS) $< | perl -p -e 's#[^:]*:#$(TARGET_DIR)/$$&#' > $@
$(DEPS_DIR)/%.d: $(SOURCE_DIR)/%.cpp Makefile | $(DEPS_DIR)
	@$(CXX) -MM -E $(CPPFLAGS) $(VPI_CPPFLAGS) $< | perl -p -e 's#[^:]*:#$(TARGET_DIR)/$$&#' > $@

-include $(DEPS)

$(TARGET_DIR)/%.o: $(SOURCE_DIR)/%.c $(DEPS_DIR)/%.d | $(TARGET_DIR) $(ASM_DIR)
	$(CC) $(ALL_CFLAGS) -g -c $< -o $@ $(GEN_ASM_FLAGS)

$(TARGET_DIR)/%.o: $(SOURCE_DIR)/%.cc $(DEPS_DIR)/%.d | $(TARGET_DIR) $(ASM_DIR)
	$(CXX) $(ALL_CXXFLAGS) -g -c $< -o $@ $(GEN_ASM_FLAGS)
$(TARGET_DIR)/%.o: $(SOURCE_DIR)/%.c++ $(DEPS_DIR)/%.d | $(TARGET_DIR) $(ASM_DIR)
	$(CXX) $(ALL_CXXFLAGS) -g -c $< -o $@ $(GEN_ASM_FLAGS)
$(TARGET_DIR)/%.o: $(SOURCE_DIR)/%.cpp $(DEPS_DIR)/%.d | $(TARGET_DIR) $(ASM_DIR)
	$(CXX) $(ALL_CXXFLAGS) -g -c $< -o $@ $(GEN_ASM_FLAGS)

$(TARGET_DIR)/%.so: $(OBJECTS)
	$(CXX) $(WARNFLAGS) $(OBJECTS) -shared $(ALL_LDFLAGS) -o $@

$(TARGET_DIR)/%.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

$(TARGET_DIR)/$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(WARNFLAGS) $(ARCHFLAGS) $(OBJECTS) $(ALL_LDFLAGS) -o $@ $(addprefix -L, $(LIB_DIR)) $(LIBS)

run: $(TARGET_DIR)/$(EXECUTABLE)
	$(TARGET_DIR)/$(EXECUTABLE) $(RUNPARAMS)

dump: $(TARGET_DIR)/$(TARGET)
	$(OD) -dxt -S $(TARGET_DIR)/$(TARGET)

release: $(TARGET_DIR)/$(TARGET)
	$(STRIP) -g $(TARGET_DIR)/$(TARGET)

strip-all: $(TARGET_DIR)/$(EXECUTABLE)
	$(STRIP) -s $(TARGET_DIR)/$(EXECUTABLE)

# ------------ extras  ---------------------------------------------------------

pkgconfig: $(PC_TRG)

%.pc: %.pc.src
	@echo "creating $@"
	@cat $< | sed \
		-e "s#%%prefix%%#$(PREFIX)#g" \
		-e "s#%%libdir%%#$(LIBDIR)#g" \
		-e "s#%%includedir%%#$(INCLUDEDIR)#g" \
		-e "s#%%version%%#$(VERSION)#g" \
		-e "s#%%requires%%#$(PKG_CFG_LIBS)#g" \
		-e "s#%%libs%%#$(VPI_LDFLAGS) $(THREAD_LDFLAGS)#g" \
		-e "s#%%cflags%%#$(VPI_CPPFLAGS) $(THREAD_CPPFLAGS)#g" \
		-e 's#\s\+$$##g' \
		> $@

doc:
	doxygen $(DOXYFILE) |grep -i warning; true

viewdoc:
	@$${BROWSER:-firefox} $(DOXY_DOCPATH)/html/index.html >/dev/null 2>&1 &

clean:
	-@$(RM) $(addprefix $(TARGET_DIR)/, *.o *.so *.a $(TARGET)) $(ASM_DIR)/*.s $(addprefix $(DEPS_DIR)/, *.d) $(PC_TRG)
	-@$(RMDIR) $(DEPS_DIR) $(TARGET_DIR) $(ASM_DIR) 2>/dev/null; true
	-@$(RM) -rf $(addprefix ./$(DOXY_DOCPATH)/, html latex rtf man); true
	-@$(RMDIR) ./$(DOXY_DOCPATH) 2>/dev/null; true

uncrustify:
	@for f in $$(find . -type f -regex '.*\.\([ch]\(pp\|++\)\|h\|inl\)') ; do \
		uncrustify -l CPP -c .uncrustify.cfg --replace --no-backup $$f ; \
	done
	@for f in $$(find . -type f -regex '.*\.c') ; do \
		uncrustify -l C -c .uncrustify.cfg --replace --no-backup $$f ; \
	done

.SECONDARY: $(DEPS) $(OBJECTS)

.PHONY: all run release strip-all doc viewdoc clean uncrustify pkgconfig

