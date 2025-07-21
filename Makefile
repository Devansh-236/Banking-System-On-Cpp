# Banking System Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
SRCDIR = src
HEADERDIR = src/headers
IMPLDIR = src/implementation
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj
BINDIR = $(BUILDDIR)/bin

# Source files
SOURCES = $(IMPLDIR)/Account.cpp \
          $(IMPLDIR)/Customer.cpp \
          $(IMPLDIR)/Transaction.cpp \
          $(IMPLDIR)/Loan.cpp \
          $(IMPLDIR)/Bank.cpp \
          $(IMPLDIR)/FileManager.cpp \
          $(IMPLDIR)/Login.cpp \
          $(IMPLDIR)/UI.cpp \
          $(IMPLDIR)/Utilities.cpp \
          $(SRCDIR)/main.cpp

# Object files
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
OBJECTS := $(OBJECTS:$(IMPLDIR)/%.cpp=$(OBJDIR)/%.o)

# Target executable
TARGET = $(BINDIR)/banking_system

# Include directories
INCLUDES = -I$(HEADERDIR)

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)
	@mkdir -p data
	@mkdir -p backup
	@mkdir -p logs

# Build target
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/%.o: $(IMPLDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILDDIR)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Install (copy to system directory)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/banking_system

# Create backup of data files
backup:
	@mkdir -p backup
	@cp data/*.dat backup/ 2>/dev/null || echo "No data files to backup"
	@echo "Backup created successfully"

# Restore from backup
restore:
	@cp backup/*.dat data/ 2>/dev/null || echo "No backup files found"
	@echo "Data restored from backup"

# Debug build
debug: CXXFLAGS += -DDEBUG -g
debug: $(TARGET)

# Release build
release: CXXFLAGS += -O3 -DNDEBUG
release: clean $(TARGET)

.PHONY: all clean run install backup restore debug release directories
