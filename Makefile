
CC = gcc
LEXER = flex
PARSER = bison
LDFLAGS = -lfl -Lbuild/

LEXDIR = flex
PARSEDIR = bison
BUILDDIR = build
SRCCODE = source

TARGET = rji
LEXTARGET = $(TARGET)lex
PARSETARGET = $(TARGET)parse

# necessary files
_TABC = $(PARSETARGET).tab.c
TABC = $(patsubst %,$(BUILDDIR)/%,$(_TABC))
_LEXC = $(LEXTARGET).yy.c
LEXC = $(patsubst %,$(BUILDDIR)/%,$(_LEXC))

LEXFILE = $(LEXTARGET).l
PARSEFILE = $(PARSETARGET).y

all: before $(TARGET)
	@echo "Target \"$(TARGET)\" built. Bailing."

before:
	[ -d $(OBJDIR) ]  || mkdir -p $(OBJDIR)

clean:
	rm -f $(BUILDDIR)/*.c $(BUILDDIR)/*.h $(BUILDDIR)/*.output $(TARGET)

test: 
	@./test.sh

$(TARGET): $(TABC) $(LEXC)
	@echo "Compiling $^"
	$(CC) -o $@ $^ $(LDFLAGS)

$(TABC): $(PARSEDIR)/$(PARSEFILE)
	@echo "Generating parser"
	$(PARSER) -dv $< -o $@

$(LEXC): $(LEXDIR)/$(LEXFILE)
	@echo "Generating lex scanner"
	$(LEXER) -o $@ $<
