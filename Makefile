CXX      := g++
CXXFLAGS := -std=c++23 -Wall -Wextra -Iinclude
DBGFLAGS := -g3 -O0 -fsanitize=address,undefined
RELFLAGS := -O2

SRCDIR   := src
BUILDDIR := build
TARGET   := npl

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SRCS))

.PHONY: all debug clean re run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(RELFLAGS) -o $@ $^

debug: $(OBJS)
	$(CXX) $(CXXFLAGS) $(DBGFLAGS) -o $(TARGET)_dbg $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: all
	./$(TARGET) $(FILE)

clean:
	rm -rf $(BUILDDIR) $(TARGET) $(TARGET)_dbg

re: clean all
