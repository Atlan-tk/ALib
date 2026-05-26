
ALIB := $(CURDIR)/
CONDIR := $(ALIB)config/
export ALIB

CONFIG ?=
export CONFIG

ifneq ($(CONFIG), )
    -include $(CONDIR)$(CONFIG).mk
else
    ifneq ($(wildcard $(ALIB).config), )
        -include $(ALIB).config
    else
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
        -include $(CONDIR)$(CONFIG).mk
    endif
endif

.PHONY: all clean disclean install uninstall

all: $(BUILD)
	@cp $(CONDIR)$(CONFIG).mk $(ALIB).config
	$(MAKE) -C $(ALIB)src DIR=$(ALIB)src/
	$(MAKE) -C $(ALIB)test DIR=$(ALIB)test/
	$(MAKE) -C $(ALIB)sample DIR=$(ALIB)sample/

clean:
	$(MAKE) -C $(ALIB)src DIR=$(ALIB)src/ clean
	$(MAKE) -C $(ALIB)test DIR=$(ALIB)test/ clean
	$(MAKE) -C $(ALIB)sample DIR=$(ALIB)sample/ clean
	@rm -rf $(BUILD)

disclean:
	@rm -rf $(ALIB).config

$(BUILD):
	@mkdir -p $(BUILD) $(OUTDIR) $(INCLUDE)
	@ln -s $(ALIB)inc $(INCLUDE)alib

install: $(TARLIB) $(INSTALL_LIB) $(INSTALL_INC)
	@cp $(ALIB)inc/*.h $(INSTALL_INC)
	@cp $(TARLIB) $(INSTALL_LIB)

uninstall:
	@rm -rf $(INSTALL_LIB)$(LIB) $(INSTALL_INC)

$(INSTALL_LIB):
	@mkdir -p $@

$(INSTALL_INC):
	@mkdir -p $@

$(TARLIB): $(BUILD)
	$(MAKE) -C $(ALIB)src DIR=$(ALIB)src/

