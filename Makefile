APPLICATION = example

BOARD ?= native

RIOTBASE ?= $(CURDIR)/../../RIOT

DEVELHELP ?= 1

USEMODULE += xtimer

# Modules to include:
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# Some boards do not initialize LED0 by default
CFLAGS=-DAUTO_INIT_LED0

include $(RIOTBASE)/Makefile.include
