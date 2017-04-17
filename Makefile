
CC = g++
LEXER = flex
PARSER = bison
CFLAGS = -std=c++11 -g -Isymtable/
LDFLAGS = -lfl -Lbuild/

LEXDIR = flex
PARSEDIR = bison
BUILDDIR = build
SEMDIR = symtable
SRCCODE = source

TARGET = rji
LEXTARGET = $(TARGET)lex
PARSETARGET = $(TARGET)parse
SEMTARGET = symtable

# necessary files
_TABC = $(PARSETARGET).tab.c
TABC = $(patsubst %,$(BUILDDIR)/%,$(_TABC))
_TABO = $(PARSETARGET).tab.o
TABO = $(patsubst %,$(BUILDDIR)/%,$(_TABO))
_LEXC = $(LEXTARGET).yy.c
LEXC = $(patsubst %,$(BUILDDIR)/%,$(_LEXC))
_LEXO = $(LEXTARGET).yy.o
LEXO = $(patsubst %,$(BUILDDIR)/%,$(_LEXO))
_SEMO = $(SEMTARGET).o
SEMO = $(patsubst %,$(BUILDDIR)/%,$(_SEMO))

LEXFILE = $(LEXTARGET).l
PARSEFILE = $(PARSETARGET).y

all: before $(TARGET)
	@echo "Target \"$(TARGET)\" built. Bailing."

before:
	[ -d $(OBJDIR) ]  || mkdir -p $(OBJDIR)

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/*.c $(BUILDDIR)/*.h $(BUILDDIR)/*.output $(TARGET)


$(TARGET): $(TABO) $(LEXO) $(SEMO)
	@echo "Linking $^"
	$(CC) -o $@ $^ $(LDFLAGS)

$(TABO): $(TABC)
	@echo "Compiling $^"
	$(CC) -o $@ -c $< $(CFLAGS)

$(LEXO): $(LEXC)
	@echo "Compiling $^"
	$(CC) -o $@ -c $< $(CFLAGS)

$(TABC): $(PARSEDIR)/$(PARSEFILE)
	@echo "Generating parser"
	$(PARSER) -dv $< -o $@

$(LEXC): $(LEXDIR)/$(LEXFILE)
	@echo "Generating lex scanner"
	$(LEXER) -o $@ $<

################################

$(SEMO): $(SEMDIR)/$(SEMTARGET).cpp
	@echo "Compiling the symbol table module"
	$(CC) -o $@ -c $<
