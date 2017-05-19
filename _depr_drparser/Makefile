## makefile for the Romaji languaje compiler ##

CC = g++
LEX = flex
CFLAGS = -std=c++14 -g -I$(DEPSDIR)
LDFLAGS = 

DEPSDIR = include
OBJDIR = obj
SRCDIR = src
GENSRCDIR = src/gen

TEST = test
COMP = rjic

TARGET = $(TEST)

## modules ##

TEST_C = $(patsubst %,$(SRCDIR)/%.cpp,$(TEST))
TEST_O = $(patsubst %,$(OBJDIR)/%.o,$(TEST))

COMP_C = $(patsubst %,$(SRCDIR)/%.cpp,$(COMP))
COMP_O = $(patsubst %,$(OBJDIR)/%.o,$(COMP))

PARSER = rjiparse
PARSER_C = $(patsubst %,$(SRCDIR)/%.cpp,$(PARSER))
PARSER_O = $(patsubst %,$(OBJDIR)/%.o,$(PARSER))

SYMTABLE = symtable
SYMTABLE_C = $(patsubst %,$(SRCDIR)/%.cpp,$(SYMTABLE))
SYMTABLE_O = $(patsubst %,$(OBJDIR)/%.o,$(SYMTABLE))

LEXER = rjilex
LEXER_L = $(patsubst %,$(SRCDIR)/%.l,$(LEXER))
LEXER_C = $(patsubst %,$(GENSRCDIR)/%.c,$(LEXER))
LEXER_O = $(patsubst %,$(OBJDIR)/%.o,$(LEXER))

## general rules ##

.PHONY: all
.PHONY: clean
.PHONY: clean_obj

all: before $(TARGET)

before:
	[ -d $(OBJDIR) ]  || mkdir -p $(OBJDIR)

clean:
	@rm -f $(TARGET) $(OBJDIR)/*.o $(GENSRCDIR)/*.c

## particular rules ##

$(TEST): $(TEST_O) $(PARSER_O) $(SYMTABLE_O) $(LEXER_O)
	@echo "Linking $@..."
	$(CC) -o $@ $^ $(LDFLAGS)

$(TEST_O): $(TEST_C)
	@echo "Compiling $@..."
	$(CC) -o $@ -c $< $(CFLAGS)

$(PARSER_O): $(PARSER_C)
	@echo "Compiling $@..."
	$(CC) -o $@ -c $< $(CFLAGS)

$(SYMTABLE_O): $(SYMTABLE_C)
	@echo "Compiling $@..."
	$(CC) -o $@ -c $< $(CFLAGS)

$(LEXER_O): $(LEXER_C)
	@echo "Compiling $@..."
	$(CC) -o $@ -c $< $(CFLAGS)

$(LEXER_C): $(LEXER_L)
	@echo "Generating $@..."
	$(LEX) -o $@ $<

## ##
