
ALIB := $(CURDIR)/
export ALIB

CONFIG ?=
export CONFIG

ifeq ($(CONFIG), )
    ifeq ($(OS),Windows_NT)
        CONFIG := windows
    else
        UNAME_S := $(shell uname -s)
        ifeq ($(UNAME_S),Linux)
            CONFIG := linux
        else ifeq ($(UNAME_S),Unix)
            CONFIG := unix
        else ifeq ($(UNAME_S),Darwin)
            CONFIG := mac
        else
            CONFIG := linux
        endif
    endif
endif

CONDIR := $(ALIB)config/
-include $(CONDIR)$(CONFIG).mk

.PHONY: all clean install remove

all: $(BUILD)
	$(MAKE) -C $(ALIB)src DIR=$(ALIB)src/
	$(MAKE) -C $(ALIB)test DIR=$(ALIB)test/
	$(MAKE) -C $(ALIB)sample DIR=$(ALIB)sample/

clean:
	$(MAKE) -C $(ALIB)src DIR=$(ALIB)src/ clean
	$(MAKE) -C $(ALIB)test DIR=$(ALIB)test/ clean
	$(MAKE) -C $(ALIB)sample DIR=$(ALIB)sample/ clean
	@rm -rf $(BUILD)

$(BUILD):
	@mkdir $(BUILD) $(OUTDIR) $(INCLUDE)
	@ln -s $(ALIB)inc $(INCLUDE)alib

install: $(TARLIB) $(INSTALL_LIB) $(INSTALL_INC)
	@cp $(ALIB)inc/*.h $(INSTALL_INC)
	@cp $(TARLIB) $(INSTALL_LIB)

remove:
	@rm -rf $(INSTALL_LIB) $(INSTALL_INC)

$(INSTALL_LIB):
	@mkdir $@

$(INSTALL_INC):
	@mkdir $@

$(TARLIB): $(BUILD)
	$(MAKE) -C $(ALIB)src DIR=$(ALIB)src/

