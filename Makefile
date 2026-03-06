# ============================================================
# UTinyDB (C++) — Makefile
# ============================================================

CXX      = g++
AR       = ar

# Directories
SRCDIR   = src
INCDIR   = include
TESTDIR  = tests
BUILDDIR = build
OBJDIR   = $(BUILDDIR)/obj
EXDIR    = examples

# Sources
SRCS     = $(wildcard $(SRCDIR)/*.cpp)
OBJS     = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

LIB      = $(BUILDDIR)/libutinydb.a

# Test
TEST_SRCS = $(TESTDIR)/test_main.cpp
TEST_BIN  = $(BUILDDIR)/test_runner

# Include paths
INCS     = -I$(INCDIR)

# Flags
CXXFLAGS_BASE  = -std=c++17 -Wall -Wextra -Wpedantic -Werror $(INCS)
CXXFLAGS_REL   = $(CXXFLAGS_BASE) -O2
CXXFLAGS_DBG   = $(CXXFLAGS_BASE) -g -O0
CXXFLAGS_ASAN  = $(CXXFLAGS_DBG) -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS_ASAN   = -fsanitize=address,undefined

# ============================================================
# Targets
# ============================================================

.PHONY: build debug test test-asan test-valgrind analyze clean check demo

# --- Build (release) ---
build: CXXFLAGS = $(CXXFLAGS_REL)
build: $(LIB)

# --- Debug ---
debug: CXXFLAGS = $(CXXFLAGS_DBG)
debug: $(LIB)

# --- Library ---
$(LIB): $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(AR) rcs $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Test (debug mode) ---
test: CXXFLAGS = $(CXXFLAGS_DBG)
test: $(LIB)
	$(CXX) $(CXXFLAGS) -I$(TESTDIR) $(TEST_SRCS) -L$(BUILDDIR) -lutinydb -o $(TEST_BIN)
	@echo ""
	@echo "=== Running tests ==="
	@$(TEST_BIN)

# --- Test with AddressSanitizer + UBSan ---
test-asan:
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS_ASAN) -I$(TESTDIR) $(SRCS) $(TEST_SRCS) $(LDFLAGS_ASAN) -o $(BUILDDIR)/test_asan
	@echo ""
	@echo "=== Running tests (ASan + UBSan) ==="
	@$(BUILDDIR)/test_asan

# --- Test with Valgrind ---
test-valgrind: CXXFLAGS = $(CXXFLAGS_DBG)
test-valgrind: $(LIB)
	$(CXX) $(CXXFLAGS) -I$(TESTDIR) $(TEST_SRCS) -L$(BUILDDIR) -lutinydb -o $(TEST_BIN)
	@echo ""
	@echo "=== Running tests (Valgrind) ==="
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 --quiet $(TEST_BIN)

# --- Static analysis (Clang) ---
analyze:
	@echo "=== Clang Static Analyzer ==="
	scan-build --status-bugs $(CXX) $(CXXFLAGS_DBG) $(SRCS) $(TEST_SRCS) -I$(TESTDIR) -o /dev/null

# --- Clean ---
clean:
	rm -rf $(BUILDDIR)

# --- Full check ---
check: test-asan test-valgrind
	@echo ""
	@echo "========================================"
	@echo "  ALL CHECKS PASSED"
	@echo "========================================"

# --- Demo ---
demo: CXXFLAGS = $(CXXFLAGS_DBG)
demo: $(LIB)
	$(CXX) $(CXXFLAGS) $(EXDIR)/demo.cpp -L$(BUILDDIR) -lutinydb -o $(BUILDDIR)/demo
	@$(BUILDDIR)/demo
