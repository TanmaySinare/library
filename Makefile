# Library Management System - build, test, coverage and analysis targets

CC       := gcc
CXX      := g++
CSTD     := -std=c11
CXXSTD   := -std=c++17
WARN     := -Wall -Wextra -Wpedantic -Wconversion -Wshadow
CFLAGS   ?= $(CSTD) $(WARN) -O0 -g
CXXFLAGS ?= $(CXXSTD) -Wall -Wextra -Wpedantic -O0 -g
INCLUDES := -Iinclude
GTEST_LIBS ?= -lgtest -lgtest_main
THREAD_FLAGS ?= -pthread

SRC_DIR      := Src
INCLUDE_DIR  := include
TEST_DIR     := Test
BUILD_DIR    := Built
COVERAGE_DIR := Coverage

C_SOURCES := $(SRC_DIR)/main.c \
             $(SRC_DIR)/auth.c \
             $(SRC_DIR)/books.c \
             $(SRC_DIR)/members.c \
             $(SRC_DIR)/storage.c \
             $(SRC_DIR)/transactions.c \
             $(SRC_DIR)/util.c

# Exclude main.c from tests to avoid linking a second main() function.
MODULE_SOURCES := $(filter-out $(SRC_DIR)/main.c,$(C_SOURCES))
APP_OBJECTS    := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
TEST_OBJECTS   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/test_%.o,$(MODULE_SOURCES))
TEST_SOURCE    := $(TEST_DIR)/library_all_function_unit_tests.cpp
APP            := $(BUILD_DIR)/library
GTEST          := $(BUILD_DIR)/library_tests
GTEST_SAN      := $(BUILD_DIR)/library_tests_sanitize
GTEST_COV      := $(BUILD_DIR)/library_tests_coverage
SAN_CFLAGS     := $(CFLAGS) -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
SAN_OBJECTS    := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/san_%.o,$(MODULE_SOURCES))
THREAD_SMOKE   := $(BUILD_DIR)/thread_search_smoke
COV_APP        := $(BUILD_DIR)/library_coverage_app
COV_MAIN_OBJ   := $(BUILD_DIR)/cov_main.o

.PHONY: all run test gtest coverage sanitize valgrind helgrind cppcheck misra misra-config-check thread-smoke valgrind-smoke helgrind-smoke clean help directories

all: $(APP)

directories:
	mkdir -p $(BUILD_DIR) $(COVERAGE_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/library.h | directories
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/library.h | directories
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $(INCLUDES) -c $< -o $@

$(APP): $(APP_OBJECTS)
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $^ -o $@

run: $(APP)
	./$(APP)

$(GTEST): $(TEST_SOURCE) $(TEST_OBJECTS) $(INCLUDE_DIR)/library.h | directories
	$(CXX) $(CXXFLAGS) $(THREAD_FLAGS) $(INCLUDES) $(TEST_SOURCE) $(TEST_OBJECTS) $(GTEST_LIBS) -o $@

test gtest: $(GTEST)
	./$(GTEST)

$(THREAD_SMOKE): Test/thread_search_smoke.c $(MODULE_SOURCES) $(INCLUDE_DIR)/library.h | directories
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $(INCLUDES) Test/thread_search_smoke.c $(MODULE_SOURCES) -o $@

thread-smoke: $(THREAD_SMOKE)
	./$(THREAD_SMOKE)

valgrind-smoke: $(THREAD_SMOKE) | directories
	command -v valgrind >/dev/null 2>&1 || { echo 'ERROR: Valgrind is not installed.'; exit 1; }
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 \
	  --log-file=$(COVERAGE_DIR)/valgrind_thread_search_report.txt ./$(THREAD_SMOKE)

helgrind-smoke: $(THREAD_SMOKE) | directories
	command -v valgrind >/dev/null 2>&1 || { echo 'ERROR: Valgrind is not installed.'; exit 1; }
	valgrind --tool=helgrind --error-exitcode=1 \
	  --log-file=$(COVERAGE_DIR)/helgrind_thread_search_report.txt ./$(THREAD_SMOKE)

# Compile C modules with GCC (as C), then link the C++ GoogleTest runner.
$(BUILD_DIR)/san_%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/library.h | directories
	$(CC) $(SAN_CFLAGS) $(THREAD_FLAGS) $(INCLUDES) -c $< -o $@

$(GTEST_SAN): $(TEST_SOURCE) $(SAN_OBJECTS) $(INCLUDE_DIR)/library.h | directories
	$(CXX) $(CXXSTD) -Wall -Wextra -Wpedantic -O1 -g $(THREAD_FLAGS) \
	  -fsanitize=address,undefined -fno-omit-frame-pointer $(INCLUDES) \
	  $(TEST_SOURCE) $(SAN_OBJECTS) $(GTEST_LIBS) -o $@

# Build and run tests with AddressSanitizer and UndefinedBehaviorSanitizer.
sanitize: $(GTEST_SAN)
	ASAN_OPTIONS=detect_leaks=1 ./$(GTEST_SAN)

valgrind: $(GTEST) | directories
	command -v valgrind >/dev/null 2>&1 || { echo 'ERROR: Valgrind is not installed.'; exit 1; }
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 \
	  --log-file=$(COVERAGE_DIR)/valgrind_report.txt ./$(GTEST)

helgrind: $(GTEST) | directories
	command -v valgrind >/dev/null 2>&1 || { echo 'ERROR: Valgrind is not installed.'; exit 1; }
	valgrind --tool=helgrind --error-exitcode=1 --log-file=$(COVERAGE_DIR)/helgrind_report.txt ./$(GTEST)

cppcheck: | directories
	command -v cppcheck >/dev/null 2>&1 || { echo 'ERROR: Cppcheck is not installed.'; exit 1; }
	cppcheck --enable=all --inconclusive --std=c11 --language=c --inline-suppr \
	  --suppress=missingIncludeSystem --error-exitcode=1 --output-file=$(COVERAGE_DIR)/cppcheck_report.txt \
	  $(INCLUDES) $(C_SOURCES)
	@echo 'Cppcheck report: $(COVERAGE_DIR)/cppcheck_report.txt'

# Derive ENOENT from the active C compiler's errno.h for Cppcheck's addon parser.
# This is an analysis-only definition; application source continues to use errno.h.
MISRA_SYSTEM_DEFINES := $(shell printf '#include <errno.h>\n' | $(CC) $(CSTD) -E -dM -x c - 2>/dev/null | awk '$$2 == "ENOENT" { print "-DENOENT=" $$3 }')

# MISRA addon screening. Findings are written to a report; a clean command exit
# is not a certification and does not mean every MISRA rule is satisfied.
misra: | directories
	command -v cppcheck >/dev/null 2>&1 || { echo 'ERROR: Cppcheck is not installed.'; exit 1; }
	@echo 'Host MISRA analysis definitions: $(MISRA_SYSTEM_DEFINES)'
	cppcheck --addon=misra.py --enable=all --inconclusive --std=c11 --language=c \
	  --platform=unix64 --suppress=missingIncludeSystem $(MISRA_SYSTEM_DEFINES) \
	  --error-exitcode=1 --output-file=$(COVERAGE_DIR)/misra_report.txt $(INCLUDES) $(C_SOURCES)
	@echo 'MISRA screening report: $(COVERAGE_DIR)/misra_report.txt'
	@echo 'Review every rule finding and approved deviation; this is not a compliance certificate.'

misra-config-check: | directories
	command -v cppcheck >/dev/null 2>&1 || { echo 'ERROR: Cppcheck is not installed.'; exit 1; }
	cppcheck --check-config --std=c11 --language=c --platform=unix64 \
	  --suppress=missingIncludeSystem $(MISRA_SYSTEM_DEFINES) $(INCLUDES) $(C_SOURCES)

# Compile the tested modules with coverage instrumentation, then run GoogleTest.
$(BUILD_DIR)/cov_%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/library.h | directories
	$(CC) $(CFLAGS) $(THREAD_FLAGS) $(INCLUDES) --coverage -c $< -o $@

COV_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/cov_%.o,$(MODULE_SOURCES))
COV_APP_OBJECTS := $(COV_MAIN_OBJ) $(COV_OBJECTS)

$(GTEST_COV): $(TEST_SOURCE) $(COV_OBJECTS) $(INCLUDE_DIR)/library.h | directories
	$(CXX) $(CXXFLAGS) $(THREAD_FLAGS) $(INCLUDES) --coverage $(TEST_SOURCE) $(COV_OBJECTS) $(GTEST_LIBS) -o $@

$(COV_APP): $(COV_APP_OBJECTS)
	$(CC) $(CFLAGS) $(THREAD_FLAGS) --coverage $^ -o $@

coverage:
	$(MAKE) clean
	$(MAKE) $(GTEST_COV) $(COV_APP)
	./$(GTEST_COV)
	sh $(TEST_DIR)/coverage_smoke.sh
	: > $(COVERAGE_DIR)/gcov_report.txt
	for file in $(BUILD_DIR)/cov_*.gcno; do \
	  gcov -b -c -o $(BUILD_DIR) "$$file" >> $(COVERAGE_DIR)/gcov_report.txt || exit $$?; \
	done
	mv -f ./*.gcov $(COVERAGE_DIR)/ 2>/dev/null || true
	@echo 'Coverage report: $(COVERAGE_DIR)/gcov_report.txt (C source modules only; inspect .gcov files for uncovered lines)'

clean:
	rm -f $(BUILD_DIR)/*.o $(APP) $(GTEST) $(GTEST_SAN) $(GTEST_COV) $(COV_APP) $(THREAD_SMOKE)
	rm -f $(BUILD_DIR)/*.gcda $(BUILD_DIR)/*.gcno $(BUILD_DIR)/san_*.o $(SRC_DIR)/*.gcda $(SRC_DIR)/*.gcno
	rm -f *.gcda *.gcno *.gcov $(COVERAGE_DIR)/*.gcov $(COVERAGE_DIR)/*_report.txt

help:
	@echo 'make             Build the application'
	@echo 'make run         Build and run the application'
	@echo 'make test        Build and run GoogleTest tests (requires GoogleTest)'
	@echo 'make sanitize    Run tests with AddressSanitizer and UBSan'
	@echo 'make valgrind    Run tests under Valgrind (requires Valgrind + GoogleTest)'
	@echo 'make helgrind    Run GoogleTests under Helgrind (requires Valgrind + GoogleTest)'
	@echo 'make thread-smoke Run a standalone test of the parallel book search'
	@echo 'make valgrind-smoke Run the parallel-search smoke test under Valgrind'
	@echo 'make helgrind-smoke Run the parallel-search smoke test under Helgrind'
	@echo 'Threaded search uses up to four read-only workers; library data must remain unchanged during each search.'
	@echo 'make cppcheck    Generate a Cppcheck report'
	@echo 'make misra       Generate a MISRA addon screening report'
	@echo 'make misra-config-check Check Cppcheck parser configuration'
	@echo 'make coverage    Run tests and generate gcov coverage data'
	@echo 'make clean       Remove generated build and report files'
