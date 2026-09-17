# LAN API contract stub — builds the real domain API handlers into a tiny
# HTTP server for the dashboard contract tests. Host/simulation only.
CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined
CPPFLAGS += -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE
COMP := $(CURDIR)/../../components

INCLUDES := -I$(COMP)/domain/include
SRC := stub_server.c \
       $(COMP)/domain/src/gm_cadence.c \
       $(COMP)/domain/src/gm_odometer.c \
       $(COMP)/domain/src/gm_ring.c \
       $(COMP)/domain/src/gm_config.c \
       $(COMP)/domain/src/gm_session_sm.c \
       $(COMP)/domain/src/gm_api.c

.PHONY: all clean
all: gm_stub

gm_stub: $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ $^ -lm

clean:
	rm -f gm_stub
