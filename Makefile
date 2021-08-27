CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -Wextra -Wtype-limits

SRCDIR=src
OBJDIR=obj
BINDIR=bin
# final executable.
EXE=$(BINDIR)/ctex

# lists of .c, .h source and .o object files
SRCS=$(wildcard $(SRCDIR)/*.c)
HEADERS=$(wildcard $(SRCDIR)/*.h)
OBJS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

# complie regular version with debugging symbols
all: CFLAGS += -g
all: $(EXE)

# compile release version with optimizations
release: CFLAGS += -o2 -DNDEBUG
release: clean
release: $(EXE)

$(EXE): $(OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $<

clean: RM+=-r
clean:
	$(RM) $(BINDIR) $(OBJDIR)

run:
	$(EXE)

$(OBJDIR) $(BINDIR):
	mkdir -p $@


# testing targets

# TESTDIR=tests
# # list of test .c source files.
# # TESTS = $(wildcard $(TESTDIR)/*.c)
# # just names of final test binaries (no .o extension)
# # TESTBINS = $(patsubst $(TESTDIR)/%.c, $(TESTDIR)/bin/%, $(TESTS))

# # _TESTOBJS=$(patsubst $(SRCDIR)/%.c, $(TESTDIR)/test_bin/%.o, $(SRCS))
# # filter out main executable from test executables
# # TESTOBJS = $(filter-out $(OBJDIR)/main.o, $(OBJS))

# test:
# 	./$(TESTDIR)/test.sh ./$(TESTDIR)/test_cmds/test_*.t

# # run all tests
# test: $(TESTDIR)/bin $(TESTBINS) $(SRCDIR)
# 	for test in$(TESTBINS) ; do ./$$test ; done

# create the .o and main executable directories as well as the 
#  test binary directory if they don't exist.
# $(OBJDIR) $(BINDIR) $(TESTDIR)/bin:
# 	mkdir -p $@