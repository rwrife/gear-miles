# Host (simulation) unit tests — run on the build machine, no hardware.
# These validate the domain logic only; they are NOT bench evidence.
CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined
COMP := $(CURDIR)/../../components

INCLUDES := -I$(COMP)/domain/include -I$(COMP)/panel_hal/include
SRC := $(COMP)/domain/src/gm_cadence.c \
       $(COMP)/domain/src/gm_odometer.c \
       $(COMP)/domain/src/gm_ring.c \
       $(COMP)/domain/src/gm_config.c \
       $(COMP)/domain/src/gm_session_sm.c \
       $(COMP)/domain/src/gm_api.c \
       $(COMP)/panel_hal/src/gm_panel.c

.PHONY: all test clean

all: test

test: test_host
	./test_host

test_host: test_main.c $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ -lm

clean:
	rm -f test_host
