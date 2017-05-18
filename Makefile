
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

PAR_O = $(BUILDDIR)/$(DRPTARGET).o
PAR_C = $(DESCDIR)/$(DRPTARGET).cpp

SYM_O = $(BUILDDIR)/$(SYMTARGET).o
SYM_C = $(SYMDIR)/$(SYMTARGET).cpp

TAB_O = $(BUILDDIR)/$(BISONTARGET).tab.o
TAB_C = $(BUILDDIR)/$(BISONTARGET).tab.c
TAB_Y = $(PARSEDIR)/$(BISONTARGET).y

LEX_O = $(BUILDDIR)/$(FLEXTARGET).yy.o
LEX_C = $(BUILDDIR)/$(FLEXTARGET).yy.c
LEX_L = $(LEXDIR)/$(FLEXTARGET).l


all: before rdparser_comp
	@echo "Target \"$(TARGET)\" built. Bailing."

before:
	[ -d $(OBJDIR) ]  || mkdir -p $(OBJDIR)

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/*.c $(BUILDDIR)/*.h $(BUILDDIR)/*.output $(DEPSDIR)/*.gch $(TARGET)

rdparser_comp: $(PAR_O) $(LEX_O) $(SYM_O)
	@echo -e "\nLinking $^"
	$(CC) -o $(TARGET) $^ $(LDFLAGS)
	@echo -e "------------"

bison_comp: $(TAB_O) $(LEX_O) $(SYM_O)
	@echo -e "\nLinking $^"
	$(CC) -o $(TARGET) $^ $(LDFLAGS)
	@echo -e "------------"

$(PAR_O): $(PAR_C)
	@echo -e "\nCompiling the recursive descent parser"
	$(CC) -o $@ -c $< $(CFLAGS)
	@echo -e "------------"

$(SYM_O): $(SYM_C)
	@echo -e "\nCompiling the symbol table module"
	$(CC) -o $@ -c $< $(CFLAGS)
	@echo -e "------------"

$(TAB_O): $(TAB_C)
	@echo -e "\nCompiling $^"
	$(CC) -o $@ -c $< $(CFLAGS)
	@echo -e "------------"

$(LEX_O): $(LEX_C)
	@echo -e "\nCompiling $^"
	$(CC) -o $@ -c $< $(CFLAGS)
	@echo -e "------------"

$(TAB_C): $(TAB_Y)
	@echo -e "\nGenerating parser"
	$(PARSER) -dv $< -o $@
	@echo -e "------------"

$(LEX_C): $(LEX_L)
	@echo -e "\nGenerating lex scanner"
	$(LEXER) -o $@ $<
	@echo -e "------------"

