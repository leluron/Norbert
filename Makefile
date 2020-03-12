VARS_OLD := $(.VARIABLES)

DEPSDIR = .deps
OBJDIR = .obj
SRCDIR = src
TESTDIR = grammar_tests

DEPFLAGS=-MT $@ -MMD -MP -MF $(DEPSDIR)/$*.d
FLAGS=-I/usr/include/antlr4-runtime/ -g -std=c++14
LIBS=-lantlr4-runtime

GRAMMARS = Norbert Bytecode
GRAMMARFILES = $(patsubst %, %.g4, ${GRAMMARS})

PARSER = $(patsubst %, %Parser, ${GRAMMARS}) $(patsubst %, %Lexer, ${GRAMMARS})

SRC = main VirtualMachine Assembler
OBJPATH = $(patsubst %, $(OBJDIR)/%.o, $(PARSER) $(SRC))

MAIN = main

PARSERDIR = $(SRCDIR)/parser
PARSERH = $(patsubst %, $(PARSERDIR)/%.h, $(PARSER))
PARSERSRC = $(patsubst %, $(PARSERDIR)/%.cpp, $(PARSER))

all: $(MAIN)

cleancompile:
	rm -f $(MAIN)
	rm -rf $(OBJDIR)
	rm -rf $(DEPSDIR)

cleanparser:
	rm -rf $(PARSERDIR)

cleantest:
	rm -rf $(TESTDIR)

clean: cleancompile cleanparser cleantest

.PHONY: clean cleancompile cleanparser cleantest

$(PARSERH) $(PARSERSRC): $(GRAMMARFILES) | $(PARSERDIR)
	antlr4 -Dlanguage=Cpp *.g4 -o src/parser -visitor

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPSDIR)/%.d $(PARSERH) | $(DEPSDIR) $(OBJDIR)
	g++ $(DEPFLAGS) -c -o $@ $< $(FLAGS)

$(OBJDIR)/%.o: $(PARSERDIR)/%.cpp $(PARSERH) | $(DEPSDIR) $(OBJDIR)
	g++ -c -o $@ $< $(FLAGS) -w

$(DEPSDIR): ; mkdir -p $@
$(OBJDIR): ; mkdir -p $@
$(PARSERDIR): ; mkdir -p $@

DEPS := $(SRC)
DEPSFILES := $(patsubst %,$(DEPSDIR)/%.d, $(DEPS))
$(DEPSFILES):
include $(wildcard $(DEPSFILES))

$(MAIN): $(OBJPATH)
	g++ -o $@ $^ $(FLAGS) $(LIBS)

$(TESTDIR)/%Parser.java: %.g4
	mkdir -p $(TESTDIR)
	antlr4 $< -o $(TESTDIR)

$(TESTDIR)/%Parser.class: $(TESTDIR)/%Parser.java
	javac $(TESTDIR)/$**.java

TESTCLASSES = $(patsubst %, $(TESTDIR)/%Parser.class, ${GRAMMARS})

test: $(TESTCLASSES)

vars:; $(foreach v, $(filter-out $(VARS_OLD) VARS_OLD,$(.VARIABLES)), $(info $(v) = $($(v)))) @#noop




