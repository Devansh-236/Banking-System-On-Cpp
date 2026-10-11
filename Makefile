# Banking System Makefile

CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -g
CPPFLAGS += -Isrc/headers -MMD -MP

SRCDIR = src
IMPLDIR = $(SRCDIR)/implementation
TESTDIR = tests
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj
BINDIR = $(BUILDDIR)/bin

# Library sources (everything except main.cpp) are shared by the app and the tests
LIB_SOURCES = $(wildcard $(IMPLDIR)/*.cpp)
LIB_OBJECTS = $(LIB_SOURCES:%.cpp=$(OBJDIR)/%.o)
MAIN_OBJECT = $(OBJDIR)/$(SRCDIR)/main.o
TEST_SOURCES = $(wildcard $(TESTDIR)/*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:%.cpp=$(OBJDIR)/%.o)

TARGET = $(BINDIR)/banking_system
TEST_TARGET = $(BINDIR)/banking_tests

# Default target
all: $(TARGET)

$(TARGET): $(LIB_OBJECTS) $(MAIN_OBJECT)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(TEST_TARGET): $(LIB_OBJECTS) $(TEST_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compile any source file into the mirrored path under build/obj
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Create runtime directories
directories:
	@mkdir -p data backup logs

# Run the program
run: $(TARGET) directories
	./$(TARGET)

# Run the scripted demo (uses a temporary data directory)
demo: $(TARGET)
	./$(TARGET) --demo

# Build and run the unit tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Clean build files
clean:
	rm -rf $(BUILDDIR)

# Install (copy to system directory)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/banking_system

# Create a timestamped backup of the data files
backup:
	@stamp=$$(date +%Y%m%d_%H%M%S); \
	if ls data/*.dat >/dev/null 2>&1; then \
		mkdir -p backup/$$stamp && cp data/*.dat backup/$$stamp/ && echo "Backup created: backup/$$stamp"; \
	else echo "No data files to back up"; fi

# Restore the most recent backup
restore:
	@latest=$$(ls -d backup/*/ 2>/dev/null | sort | tail -n 1); \
	if [ -n "$$latest" ]; then cp "$$latest"*.dat data/ && echo "Data restored from $$latest"; \
	else echo "No backups found"; fi

# Debug build
debug: CXXFLAGS += -DDEBUG -O0 -fsanitize=address,undefined
debug: clean $(TARGET)

# Release build
release: CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -DNDEBUG
release: clean $(TARGET)

-include $(LIB_OBJECTS:.o=.d) $(MAIN_OBJECT:.o=.d) $(TEST_OBJECTS:.o=.d)

.PHONY: all clean run demo test install backup restore debug release directories
