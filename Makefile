CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2 -g
AR      := ar
ARFLAGS := rcs

SRCDIR  := src
OBJDIR  := obj
BINDIR  := bin

CS_SRCS := cs_value.c cs_lexer.c cs_parser.c cs_vm.c cs_stdlib.c
CS_OBJS := $(patsubst %.c,$(OBJDIR)/%.o,$(CS_SRCS))

CLI_SRCS := main.c
CLI_OBJS := $(patsubst %.c,$(OBJDIR)/%.o,$(CLI_SRCS))

LIB := $(BINDIR)/libcupidscript.a
BIN := $(BINDIR)/cupidscript

.PHONY: all clean dirs

all: dirs $(LIB) $(BIN)

dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(LIB): $(CS_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(BIN): $(CS_OBJS) $(CLI_OBJS)
	$(CC) $(CFLAGS) -Isrc $^ -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

