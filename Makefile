#
# iverilog + stimc(optional) Makefile
#


# ------------  verilog testbench + sources  -----------------------------------
SIM_NAME        = tb_ff
TOPLEVEL        = $(SIM_NAME) $(DUMP_MOD_NAME)
VLOG_SOURCES    = $(wildcard ./ff-verilog/*.v)
VLOG_INCDIRS    =

# ------------  stimc_sources  -------------------------------------------------
STIMC_MODULES   = ff_emulator
STIMC_DIRS      = ./stimc++ ./ff-stimc++/
STIMC_SOURCES   = $(wildcard $(addsuffix /*.c*, $(STIMC_DIRS)))

# ------------  gtkwavefile & dumpfile  ----------------------------------------
GTKWAVEFILE     = $(SIM_NAME).gtkw
DUMPFILE        = $(WORK_DIR)/$(SIM_NAME).dump

# ------------  configuration parameters ---------------------------------------
DUMPER          = fst
GTKWAVE_LOG     = $(WORK_DIR)/gtkwave.log
VVP_LOG         = $(WORK_DIR)/$(SIM_NAME).log
DUMP_MOD        = dump.v
DUMP_MOD_NAME   = $(notdir $(basename $(DUMP_MOD)))

#-------------  working directory  ---------------------------------------------
WORK_DIR       ?= .work

# ------------  name of vvp file  ----------------------------------------------
VVP_FILE        = $(WORK_DIR)/$(SIM_NAME)-simulate

#===============================================================================
# The following statements usually need not to be changed
#===============================================================================

USE_TEMP        = 1
TMPDIR         ?= /tmp
COMPILE_DEPS = $(foreach incdir, $(INCLUDEDIRS), $(wildcard $(incdir)/*.vh))

# ------------  generate the names of the stimc related files  -----------------
STIMC_BASENAMES = $(notdir $(basename $(STIMC_SOURCES)))
STIMC_OBJECTS   = $(addprefix $(WORK_DIR)/, $(addsuffix .o, $(STIMC_BASENAMES)))
STIMC_DEPS      = $(addprefix $(WORK_DIR)/, $(addsuffix .d, $(STIMC_BASENAMES)))

# ------------  build and run tools  -------------------------------------------
IVERILOG        = iverilog
GP             ?=
CC              = $(GP)gcc
CXX             = $(GP)g++
LD              = $(GP)ld
LN              = ln -sf
VVP             = vvp
MKTEMP          = mktemp -p $(TMPDIR) -t ivl-$(USER)-$(SIM_NAME)-XXXX

# ------------  tool flags for iverilog  ---------------------------------------
IVERLILOG_FLAGS = -Wall -Wno-timescale -Wno-implicit-dimensions \
                  -Wno-sensitivity-entire-array -Wno-portbind \
                  -g2005-sv

# ------------  tool flags for vvp  --------------------------------------------
VVP_FLAGS       = -n -i -l$(VVP_LOG)
VVP_EXTARGS     =
VVP_EXTARGS_RUN = -none

# ------------  tool flags for stimc  ------------------------------------------
STIMC_LDFLAGS   = -lpcl

WARNFLAGS      ?= -Wall -Wextra -Wshadow
OPTFLAGS       ?= -O2 -fdata-sections -ffunction-sections
ARCHFLAGS      ?= -mtune=native -march=native
CFLAGS         ?= -fpic -Wstrict-prototypes
CXXFLAGS       ?= -fpic
CPPFLAGS       ?= $(addprefix -I,$(STIMC_DIRS))
LDFLAGS        ?= -lpcl

ifneq ($(strip $(STIMC_MODULES)),)
  VPI_CFLAGS    = $(shell iverilog-vpi --cflags | egrep -o -- '-I\s*\S*')
  VPI_LDFLAGS   = $(shell iverilog-vpi --ldflags)
  VPI_MODULE   ?= $(WORK_DIR)/stimc_model.vpi
  VVP_FLAGS    += -M$(WORK_DIR) -m$(notdir $(basename $(VPI_MODULE)))
endif

ALL_CFLAGS      = $(WARNFLAGS) $(OPTFLAGS) $(ARCHFLAGS) $(VPI_CFLAGS) $(CFLAGS)   $(CPPFLAGS)
ALL_CXXFLAGS    = $(WARNFLAGS) $(OPTFLAGS) $(ARCHFLAGS) $(VPI_CFLAGS) $(CXXFLAGS) $(CPPFLAGS)
ALL_LDFLAGS     = $(VPI_LDFLAGS) $(LDFLAGS)

VPATH = $(subst $() $(),:,$(STIMC_DIRS))

all: rungui

$(WORK_DIR):
ifeq ($(USE_TEMP),1)
	@$(LN) `$(MKTEMP) -d` $(WORK_DIR)
else
	@mkdir -p $(WORK_DIR)
endif

$(WORK_DIR)/%.d: %.c | $(WORK_DIR)
	@$(CC) -MM -E $(CPPFLAGS) $< | perl -p -e 's#[^:]*:#$(WORK_DIR)/$$&#' > $@
	@$(LN) $*.d $(WORK_DIR)/$*.dep

$(WORK_DIR)/%.o: %.c $(WORK_DIR)/%.d $(MAKEFILE_LIST) | $(WORK_DIR)
	$(CC) $(ALL_CFLAGS) -g -c $< -o $@

$(WORK_DIR)/%.d: %.cpp | $(WORK_DIR)
	@$(CXX) -MM -E $(CPPFLAGS) $< | perl -p -e 's#[^:]*:#$(WORK_DIR)/$$&#' > $@
	@$(LN) $*.d $(WORK_DIR)/$*.dep

$(WORK_DIR)/%.o: %.cpp $(WORK_DIR)/%.d $(MAKEFILE_LIST) | $(WORK_DIR)
	$(CXX) $(ALL_CXXFLAGS) -g -c $< -o $@

-include ${STIMC_DEPS:.d=.dep}

$(WORK_DIR)/%.vpi: $(STIMC_OBJECTS)
	$(CC) $(WARNFLAGS) $(ALL_LDFLAGS) -o $@ $(STIMC_OBJECTS)


$(VVP_FILE): $(VLOG_SOURCES) $(DUMP_MOD) $(COMPILE_DEPS) $(filter-out %.dep,$(MAKEFILE_LIST)) | $(WORK_DIR)
	$(IVERILOG) $(addprefix -s, $(TOPLEVEL)) $(addprefix -I,$(VLOG_INCDIRS)) $(IVERLILOG_FLAGS) $(ADDITIONAL_IVERILOG_FLAGS) $(VLOG_SOURCES) $(DUMP_MOD) -o $@


$(DUMPFILE): $(VVP_FILE) $(VPI_MODULE) $(RUN_DEPS) $(MAKEFILE_LIST)
	IVERILOG_DUMPER=${DUMPER} $(VVP) $(VVP_FLAGS) $(VVP_FILE) $(DUMPMODE) $(VVP_EXTARGS)

vlog: $(VVP_FILE)

vpi_run: $(VPI_MODULE)

elab: vlog vpi_run

run: $(VVP_FILE) $(VPI_MODULE) Makefile
	IVERILOG_DUMPER=${DUMPER} $(VVP) $(VVP_FLAGS) $(VVP_FILE) $(DUMPMODE) $(VVP_EXTARGS_RUN)

rungui: $(DUMPFILE)
	@$(MAKE) --no-print-directory gui

gui:
	gtkwave -a $(GTKWAVEFILE) -f $(DUMPFILE) -O $(GTKWAVE_LOG) &

clean:
	@rm -f \
        $(VVP_FILE) $(DUMPFILE) $(GTKWAVE_LOG) $(VVP_LOG)  \
        $(VPI_MODULE) $(STIMC_DEPS) ${STIMC_DEPS:.d=.dep} $(STIMC_OBJECTS) \
        2> /dev/null || true
	@rm -f $(WORK_DIR)/$(DUMPFILE).hier
	@if [ -L $(WORK_DIR) ]; then  \
		rm -Irf `readlink -e $(WORK_DIR)` ;\
		rm $(WORK_DIR) ;\
	elif [ -e $(WORK_DIR) ]; then \
		rmdir $(WORK_DIR) 2>&1 > /dev/null || true ;\
	fi

$(DUMP_MOD):
	@echo 'module $(DUMP_MOD_NAME) ();'     > $@
	@echo 'initial begin'                  >> $@
	@echo '    $$dumpfile("$(DUMPFILE)");' >> $@
	@echo '    $$dumpvars();'              >> $@
	@echo 'end'                            >> $@
	@echo 'endmodule'                      >> $@

.PHONY: all elab vlog vpi_run elab run rungui gui clean

.SECONDARY: $(STIMC_DEPS) $(STIMC_OBJECTS) $(VPI_MODULE)
