CXX      := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -Iinclude -finput-charset=UTF-8 -fexec-charset=UTF-8
DBGFLAGS := -g3 -O0 -fsanitize=address,undefined
RELFLAGS := -O2

SRCDIR   := src
RUNTIMEDIR := src/runtime
BUILDDIR := build
TARGET   := npl

SRCS := $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(RUNTIMEDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SRCS))
OBJS := $(OBJS:$(RUNTIMEDIR)/%.cpp=$(BUILDDIR)/runtime/%.o)

LIBS := -lncursesw

.PHONY: all debug clean re run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(RELFLAGS) -o $@ $^ $(LIBS)
	sudo cp $(TARGET) /usr/local/bin/$(TARGET)

debug: $(OBJS)
	$(CXX) $(CXXFLAGS) $(DBGFLAGS) -o $(TARGET)_dbg $^ $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: all
	./$(TARGET) $(FILE)

clean:
	rm -rf $(BUILDDIR) $(TARGET) $(TARGET)_dbg

re: clean all

asm: debug
	./$(TARGET)_dbg $(FILE)
	nasm -f elf64 output.asm -o output.o
	ld output.o -o output

runasm: asm
	./output
