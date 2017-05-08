
CC = g++
LEXER = flex
PARSER = bison
CFLAGS = -std=c++11 -g -Iinclude/
LDFLAGS = 

BUILDDIR = build
DEPSDIR = include
LEXDIR = flex
PARSEDIR = bison
DESCDIR = rdparser
SYMDIR = symtable

TARGET = rjicomp
FLEXTARGET = rjilex
BISONTARGET = rjiparse
DRPTARGET = parser
SYMTARGET = symtable

# necessary files

TAB_O = $(BUILDDIR)/$(BISONTARGET).tab.o
TAB_C = $(BUILDDIR)/$(BISONTARGET).tab.c
TAB_Y = $(PARSEDIR)/$(BISONTARGET).y

PAR_O = $(BUILDDIR)/$(DRPTARGET).o
PAR_C = $(DESCDIR)/$(DRPTARGET).cpp

SYM_O = $(BUILDDIR)/$(SYMTARGET).o
SYM_C = $(SYMDIR)/$(SYMTARGET).cpp

LEX_O = $(BUILDDIR)/$(FLEXTARGET).yy.o
LEX_C = $(BUILDDIR)/$(FLEXTARGET).yy.c
LEX_L = $(LEXDIR)/$(FLEXTARGET).l


all: before bison_comp
	@echo "Target \"$(TARGET)\" built. Bailing."

before:
	[ -d $(OBJDIR) ]  || mkdir -p $(OBJDIR)

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/*.c $(BUILDDIR)/*.h $(BUILDDIR)/*.output $(DEPSDIR)/*.gch $(TARGET)

rdparser_comp: $(PAR_O) $(LEX_O) $(SEM_O)
	@echo -e "\nLinking $^"
	$(CC) -o $(TARGET) $^ $(LDFLAGS)

bison_comp: $(TAB_O) $(LEX_O) $(SEM_O)
	@echo -e "\nLinking $^"
	$(CC) -o $(TARGET) $^ $(LDFLAGS)

$(PAR_O): $(PAR_C)
	@echo -e "\nCompiling the recursive descent parser"
	$(CC) -o $@ -c $< $(CFLAGS)

$(SYM_O): $(SYM_C)
	@echo -e "\nCompiling the symbol table module"
	$(CC) -o $@ -c $< $(CFLAGS)

$(TAB_O): $(TAB_C)
	@echo -e "\nCompiling $^"
	$(CC) -o $@ -c $< $(CFLAGS)

$(LEX_O): $(LEX_C)
	@echo -e "\nCompiling $^"
	$(CC) -o $@ -c $< $(CFLAGS)

$(TAB_C): $(TAB_Y)
	@echo -e "\nGenerating parser"
	$(PARSER) -dv $< -o $@

$(LEX_C): $(LEX_L)
	@echo -e "\nGenerating lex scanner"
	$(LEXER) -o $@ $<

