# Makefile for IsotopeReader / NetworkReader demo programs
#
# Targets:
#   make            - build all executables (IsotopeReader-demo, NetworkReader-demo, mesa-meallicity-computer)
#   make IsotopeReader-demo
#   make NetworkReader-demo
#   make mesa-meallicity-computer
#   make clean      - remove object files and executables
# Object files (.o) are placed in obj/, dependency files (.d) in deps/.
# Both directories are created automatically if they don't already exist.
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
DEPFLAGS := -MMD -MP

OBJDIR  := obj
DEPDIR  := deps
BINDIR  := bin

# ---------------------------------------------------------------------------
# isotope_demo: reads isotopes.dat via IsotopeReader
# ---------------------------------------------------------------------------
ISOTOPE_TARGET := $(BINDIR)/IsotopeReader-demo
ISOTOPE_SRCS   := IsotopeReader-demo.cpp IsotopeReader.cpp
ISOTOPE_OBJS   := $(addprefix $(OBJDIR)/,$(ISOTOPE_SRCS:.cpp=.o))
ISOTOPE_DEPS   := $(addprefix $(DEPDIR)/,$(ISOTOPE_SRCS:.cpp=.d))

# ---------------------------------------------------------------------------
# net_demo: reads MESA .net files via NetworkReader
# ---------------------------------------------------------------------------
NET_TARGET := $(BINDIR)/NetworkReader-demo
NET_SRCS   := NetworkReader-demo.cpp NetworkReader.cpp
NET_OBJS   := $(addprefix $(OBJDIR)/,$(NET_SRCS:.cpp=.o))
NET_DEPS   := $(addprefix $(DEPDIR)/,$(NET_SRCS:.cpp=.d))

# ---------------------------------------------------------------------------
# net_demo: reads MESA .net files via NetworkReader
# ---------------------------------------------------------------------------
METALLICITY_TARGET := $(BINDIR)/mesa-metallicity-computer
METALLICITY_SRCS   := mesa-metallicity-computer.cpp NetworkReader.cpp  IsotopeReader.cpp
METALLICITY_OBJS   := $(addprefix $(OBJDIR)/,$(METALLICITY_SRCS:.cpp=.o))
METALLICITY_DEPS   := $(addprefix $(DEPDIR)/,$(METALLICITY_SRCS:.cpp=.d))

.PHONY: all clean

all: $(ISOTOPE_TARGET) $(NET_TARGET) $(METALLICITY_TARGET)

$(ISOTOPE_TARGET): $(ISOTOPE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(NET_TARGET): $(NET_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(METALLICITY_TARGET): $(METALLICITY_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Generic compile rule: source files live alongside the Makefile,
# objects go in obj/, dependency files go in deps/. The directories
# are order-only prerequisites, so their mtimes don't trigger
# needless recompiles, but they're created if missing before any
# compile runs.
$(OBJDIR)/%.o: %.cpp | $(BINDIR) $(OBJDIR) $(DEPDIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -MF $(DEPDIR)/$*.d -c $< -o bin/$@
 
$(OBJDIR):
	mkdir -p $(OBJDIR)
 
$(BINDIR):
	mkdir -p $(BINDIR)

$(DEPDIR):
	mkdir -p $(DEPDIR)

clean:
	rm -f $(ISOTOPE_OBJS) $(NET_OBJS) $(METALLICITY_OBJS) $(ISOTOPE_DEPS) $(NET_DEPS) $(METALLICITY_DEPS) $(ISOTOPE_TARGET) $(NET_TARGET) $(METALLICITY_TARGET)

# Pull in auto-generated dependency files (header changes trigger rebuilds)
-include $(ISOTOPE_DEPS) $(NET_DEPS) $(METALLICITY_DEPS)
