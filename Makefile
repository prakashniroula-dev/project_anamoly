########################################################################
############################ Configuration #############################
########################################################################

CXX := g++

APPNAME := myapp
TARGET := bin/$(APPNAME)

SRCDIR := src
OBJDIR := obj
BINDIR := bin

EXT := .cpp

# SFML
SFML := SFML
INCLUDES := -I$(SFML)/include -I$(SRCDIR)
LIBDIR := -L$(SFML)/lib

# Compiler flags
CXXFLAGS := -std=c++17 -Wall -MMD -MP $(INCLUDES)

# Linker flags
LDFLAGS := $(LIBDIR) \
	-lsfml-graphics \
	-lsfml-window \
	-lsfml-system

########################################################################
######################### Source Collection ############################
########################################################################

# Recursive wildcard function (no shell)
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

# Find all .cpp files recursively under SRCDIR
SRC := $(call rwildcard,$(SRCDIR)/,*$(EXT))

# Convert to object paths under OBJDIR (preserving subdirs)
OBJ := $(patsubst $(SRCDIR)/%$(EXT),$(OBJDIR)/%.o,$(SRC))

# Dependency files
DEP := $(OBJ:.o=.d)

########################################################################
############################### Targets ################################
########################################################################

all: $(TARGET)

$(TARGET): $(OBJ) | $(BINDIR)
	$(CXX) $^ -o $@ $(LDFLAGS)

# Compile each .cpp into .o, creating the target directory if needed
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Include generated dependency files
-include $(DEP)

########################################################################
################################ Clean #################################
########################################################################

.PHONY: all clean

clean:
	rm -f $(TARGET)
	rm -rf $(OBJDIR)