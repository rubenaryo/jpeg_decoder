.PHONY: all clean help

CXX=gcc -std=c99
FLAGS=-Wall -Wextra -Werror -pedantic -Wno-unused-parameter -c -g

BUILDDIR=build
SOURCEDIR=src
EXEC=main
SOURCES:=$(wildcard $(SOURCEDIR)/*.c)
OBJ:=$(patsubst $(SOURCEDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

all: dir $(BUILDDIR)/$(EXEC)

dir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(EXEC): $(OBJ)
	$(CXX) $^ -lm -o $@

$(OBJ): $(BUILDDIR)/%.o : $(SOURCEDIR)/%.c
	$(CXX) $(FLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/$(EXEC)

help:
	@echo "Usage: make {all|clean|help}" 1>&2 && false
