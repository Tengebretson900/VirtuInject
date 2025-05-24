#-------------------------------------------------------------------------------
# Clear built-in suffix rules
.SUFFIXES:

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment (e.g. export DEVKITPPC=<path>/devkitPPC)")
endif

include $(DEVKITPPC)/wii_rules

#-------------------------------------------------------------------------------
# Project settings (use folder name as target, output to build/)
TARGET      := $(notdir $(CURDIR))
BUILD       := build
SOURCES     := source
INCLUDES    := include
DATA        := data

#-------------------------------------------------------------------------------
# Compilation flags (modify as needed)
CFLAGS      := -Wall -O2 $(MACHDEP) $(INCLUDE)
CXXFLAGS    := $(CFLAGS)
LDFLAGS     := -Wl,-Map,$(notdir $@).map $(MACHDEP)

#-------------------------------------------------------------------------------
# Library paths and libraries
LIBDIRS     := $(PORTLIBS)
LIBS        := -lgrrlib -lpngu -lpng -lz -lfat -logc

#-------------------------------------------------------------------------------
# Embed binary data from data/ using bin2o (generates .o and header)
%.tpl.o %_tpl.h : %.tpl
	@echo Converting $<
	@$(bin2o)

%.ttf.o %_ttf.h : %.ttf
	@echo Converting $<
	@$(bin2o)

%.png.o %_png.h : %.png
	@echo Converting $<
	@$(bin2o)
