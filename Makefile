CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -O2 -g -D_POSIX_C_SOURCE=200809L
LDFLAGS ?=
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

API_TEST_BIN := $(BINDIR)/c_api_tests
API_TEST_OBJS := $(OBJDIR)/c_api_tests.o

# If objects are built with gcov instrumentation, the link step must also include
# coverage flags (otherwise __gcov_* symbols are undefined).
COVERAGE_LDFLAGS :=
ifneq (,$(filter --coverage,$(CFLAGS)))
	COVERAGE_LDFLAGS += --coverage
endif
ifneq (,$(filter -fprofile-arcs,$(CFLAGS)))
	COVERAGE_LDFLAGS += -fprofile-arcs
endif
ifneq (,$(filter -ftest-coverage,$(CFLAGS)))
	COVERAGE_LDFLAGS += -ftest-coverage
endif

.PHONY: all clean dirs test

all: dirs $(LIB) $(BIN)

dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(LIB): $(CS_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(BIN): $(CS_OBJS) $(CLI_OBJS)
	$(CC) $(CFLAGS) -Isrc $^ -o $@ $(COVERAGE_LDFLAGS) $(LDFLAGS) -lm

$(OBJDIR)/c_api_tests.o: tests/c_api_tests.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

$(API_TEST_BIN): $(CS_OBJS) $(API_TEST_OBJS)
	$(CC) $(CFLAGS) -Isrc $^ -o $@ $(COVERAGE_LDFLAGS) $(LDFLAGS) -lm

clean:
	rm -rf $(OBJDIR) $(BINDIR)


test: all $(API_TEST_BIN)
	@bash tests/run_tests.sh
