##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
## Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

PREFIX ?= arm-none-eabi
CC = $(PREFIX)-gcc
LD = $(PREFIX)-gcc
OBJCOPY = $(PREFIX)-objcopy
OBJDUMP = $(PREFIX)-objdump
GDB = $(PREFIX)-gdb

TOOLCHAIN_DIR := $(shell dirname `which $(CC)`)/../$(PREFIX)
OPENCM3_DIR = libopencm3

ARCH_FLAGS = -mthumb -mcpu=cortex-m3 -msoft-float
CFLAGS += -Os -g -std=c99 \
          -Wall -Wextra -Wimplicit-function-declaration \
          -Wredundant-decls -Wstrict-prototypes \
          -Wundef -Wshadow \
          -fno-common $(ARCH_FLAGS) -MD -DSTM32F1 \
          -I$(OPENCM3_DIR)/include
LDSCRIPT = STM32F103RCT6.ld
LDFLAGS += --static -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group \
           -T$(LDSCRIPT) -nostartfiles -Wl,--gc-sections \
           $(ARCH_FLAGS) -mfix-cortex-m3-ldrd \
           -L$(OPENCM3_DIR)/lib 

# Be silent by default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
# Do not print "Entering directory ...".
MAKEFLAGS += --no-print-directory
NULL := >/dev/null
else
LDFLAGS += -Wl,--print-gc-sections
endif

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

.SUFFIXES: .elf .bin .o .c

default: stenosaurus.bin
all: default

lib:
	$(Q)if [ ! "`ls -A libopencm3`" ] ; then \
		printf "######## ERROR ########\n"; \
		printf "\tlibopencm3 is not initialized.\n"; \
		printf "\tPlease run:\n"; \
		printf "\t$$ git submodule init\n"; \
		printf "\t$$ git submodule update\n"; \
		printf "\tbefore running make.\n"; \
		printf "######## ERROR ########\n"; \
		exit 1; \
	fi
	$(Q)$(MAKE) $(MAKEFLAGS) -C libopencm3 $(NULL)

%.o: %.c
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

%.elf: $(OBJECTS) $(LDSCRIPT) lib
	$(Q)$(LD) -o $@ $(OBJECTS) -lopencm3_stm32f1 $(LDFLAGS)

%.bin: %.elf
	$(Q)$(OBJCOPY) -Obinary $< $@

flash: stenosaurus.bin
	$(Q)st-flash write $< 0x08000000

clean:
	$(Q)rm -f *.o
	$(Q)rm -f *.d
	$(Q)rm -f *.elf
	$(Q)rm -f *.bin

.PHONEY : default all flash clean
