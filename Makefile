.PHONY: all clean help

CXX=g++ -std=c++11
FLAGS=-Wall -Wextra -Werror -pedantic -Wno-unused-parameter -c -g

BUILDDIR=build
SOURCEDIR=src
EXEC=main
SOURCES:=$(wildcard $(SOURCEDIR)/*.cpp)
OBJ:=$(patsubst $(SOURCEDIR)/%.cpp,$(BUILDDIR)/%.o,$(SOURCES))

all: dir $(BUILDDIR)/$(EXEC)

dir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/$(EXEC): $(OBJ)
	$(CXX) $^ -o $@

$(OBJ): $(BUILDDIR)/%.o : $(SOURCEDIR)/%.cpp
	$(CXX) $(FLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/$(EXEC)

help:
	@echo "Usage: make {all|clean|help}" 1>&2 && false
