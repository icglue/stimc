#
# iverilog + stimc(optional) Makefile
#

# ------------  verilog testbench + sources  -----------------------------------
VLOG_DIR       ?= apb-verilog
TESTBENCH      = $(wildcard $(VLOG_DIR)/tb_*.v)
VLOG_SOURCES   = $(wildcard $(VLOG_DIR)/*.v)

# ------------  stimc_sources  -------------------------------------------------
STIMC_MODULES  = apb_emulator
STIMC_DIRS     = stimc stimc-apb
STIMC_SOURCES  = $(wildcard $(addsuffix /*.c, $(STIMC_DIRS)))

# ------------  gtkwavefile & dumpfile  ----------------------------------------
GTKWAVEFILE    = $(SIMNAME).gtkw
DUMPFILE       = $(WORK_DIR)/$(SIMNAME).dump

# ------------  configuration parameters ---------------------------------------
DUMPER         = fst
DUMP_MODULE    = dump.v
GTKWAVE_LOG    = $(WORK_DIR)/gtkwave.log
VVP_LOG        = $(WORK_DIR)/$(SIMNAME).log

#-------------  working directory  ---------------------------------------------
WORK_DIR      ?= ./work

# ------------  name of vvp file  ----------------------------------------------
VVP_FILE            = $(WORK_DIR)/$(SIMNAME)-simulate

#===============================================================================
# The following statements usually need not to be changed
#===============================================================================

SIMNAME        ?= $(notdir $(basename $(TESTBENCH)))

# ------------  generate the names of the stimc related files  -----------------
STIMC_BASENAMES = $(notdir $(basename $(STIMC_SOURCES)))
STIMC_OBJECTS   = $(addprefix $(WORK_DIR)/, $(addsuffix .o, $(STIMC_BASENAMES)))
STIMC_DEPS      = $(addprefix $(WORK_DIR)/, $(addsuffix .d, $(STIMC_BASENAMES)))

# ------------  build and run tools  -------------------------------------------
IVERILOG         = iverilog
GP              ?=
CC               = $(GP)gcc
CXX              = $(GP)g++
LD               = $(GP)ld
LN               = ln -sf
VVP              = vvp

# ------------  tool flags for iverilog  ---------------------------------------

IVERLILOG_FLAGS = -Wall -Wno-timescale -Wno-implicit-dimensions

# ------------  tool flags for vvp  --------------------------------------------
VVP_FLAGS       = -n -i -l$(VVP_LOG)
VVP_EXTARGS     =
VVP_EXTARGS_RUN = run

# ------------  tool flags for stimc  ------------------------------------------
STIMC_CFLAGS    = $(addprefix -I,$(STIMC_DIRS))
STIMC_LDFLAGS   = -lpcl

WARNFLAGS       ?= -Wall -Wextra -Wshadow -Wstrict-prototypes
OPTFLAGS        ?= -O2 -fdata-sections -ffunction-sections
ARCHFLAGS       ?= -mtune=native -march=native
CFLAGS          ?= -fpic
CXXFLAGS        ?= -fpic
CPPFLAGS        ?=
LDFLAGS         ?=

ifneq ($(strip $(STIMC_MODULES)),)
  VPI_CFLAGS   = $(shell iverilog-vpi --cflags | egrep -o -- '-I\s*\S*')
  VPI_LDFLAGS  = $(shell iverilog-vpi --ldflags)
  VPI_MODULE  ?= $(WORK_DIR)/stimc_model.vpi
  VVP_FLAGS   += -M$(WORK_DIR) -m$(notdir $(basename $(VPI_MODULE)))
endif

ALL_CFLAGS       = $(WARNFLAGS) $(OPTFLAGS) $(ARCHFLAGS) $(VPI_CFLAGS) $(STIMC_CFLAGS) $(CFLAGS) $(CPPFLAGS)
ALL_CXXFLAGS     = $(WARNFLAGS) $(OPTFLAGS) $(ARCHFLAGS) $(CXXFLAGS) $(CPPFLAGS)
ALL_LDFLAGS      = $(VPI_LDFLAGS) $(STIMC_LDFLAGS) $(LDFLAGS)

VPATH = $(subst $() $(),:,$(STIMC_DIRS))

all: rungui

$(WORK_DIR):
	@mkdir -p $@

$(WORK_DIR)/%.d: %.c $(MAKEFILE) | $(WORK_DIR)
	@$(CC) -MM -E $(CPPFLAGS) $< | perl -p -e 's#[^:]*:#$(WORK_DIR)/$$&#' > $@
	@$(LN) $*.d $(WORK_DIR)/$*.dep

$(WORK_DIR)/%.d: %.cc $(MAKEFILE) | $(WORK_DIR)
	@$(CXX) -MM -E $(CPPFLAGS) $< | perl -p -e 's#[^:]*:#$(WORK_DIR)/$$&#' > $@
	@$(LN) $*.d $(WORK_DIR)/$*.dep

-include ${STIMC_DEPS:.d=.dep}

$(WORK_DIR)/%.o: %.c $(WORK_DIR)/%.d | $(WORK_DIR)
	$(CC) $(ALL_CFLAGS) -g -c $< -o $@

$(WORK_DIR)/%.o: %.cc $(WORK_DIR)/%.d | $(WORK_DIR)
	$(CXX) $(ALL_CXXFLAGS) -g -c $< -o $@

$(WORK_DIR)/%.vpi: $(STIMC_OBJECTS)
	$(CC) $(WARNFLAGS) $(ALL_LDFLAGS) -o $@ $(STIMC_OBJECTS)


$(VVP_FILE): $(TESTBENCH) $(VLOG_SOURCES) $(DUMP_MODULE) | $(WORK_DIR)
	$(IVERILOG) ${IVERLILOG_FLAGS} $^ -o $@


$(DUMPFILE): $(VVP_FILE) $(VPI_MODULE) $(MAKEFILE)
	IVERILOG_DUMPER=${DUMPER} $(VVP) $(VVP_FLAGS) $(VVP_FILE) $(DUMPMODE) $(VVP_EXTARGS)

vlog: $(VVP_FILE)

vpi_run: $(VPI_MODULE)

elab: vlog vpi_run

run: $(VVP_FILE) $(VPI_MODULE) Makefile
	$(VVP) $(VVP_FLAGS) $(VVP_FILE) $(DUMPMODE) $(VVP_EXTARGS_RUN)

rungui: $(DUMPFILE)
	@$(MAKE) -s gui

gui:
	@if [ -e $(GTKWAVEFILE) ]; then                                           \
		eval "gtkwave $(GTKWAVEFILE) > $(GTKWAVE_LOG) 2>&1 &";                \
	else                                                                      \
		eval "gtkwave -a $(GTKWAVEFILE) $(DUMPFILE) > $(GTKWAVE_LOG) 2>&1 &"; \
	fi

clean:
	@rm -f \
        $(VVP_FILE) $(DUMPFILE) $(DUMP_MODULE) $(GTKWAVE_LOG) $(VVP_LOG)  \
        $(VPI_MODULE) $(STIMC_DEPS) ${STIMC_DEPS:.d=.dep} $(STIMC_OBJECTS) \
        2> /dev/null || true
	@rmdir $(WORK_DIR)

$(DUMP_MODULE):
	@echo 'module dump ();'                 > $@
	@echo 'initial begin'                  >> $@
	@echo '    $$dumpfile("$(DUMPFILE)");' >> $@
	@echo '    $$dumpvars();'              >> $@
	@echo 'end'                            >> $@
	@echo 'endmodule'                      >> $@

.PHONY: all elab vlog vpi_run elab run rungui gui clean

.SECONDARY: $(STIMC_DEPS) $(STIMC_OBJECTS) $(VPI_MODULE)
