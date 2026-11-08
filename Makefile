CXX ?= g++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O0 -g
COVERAGE_FLAGS ?= --coverage
LDFLAGS += $(COVERAGE_FLAGS)
PREFIX ?= /usr/local

TARGET := bin/lab
UNIT_TARGET := bin/unit_tests
INTEGRATION_TARGET := bin/integration_tests
COVERAGE_TARGET := bin/check_coverage
SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp,build/%.o,$(SOURCES))
LIB_OBJECTS := $(filter-out build/l1_UserInterface.o,$(OBJECTS))
DEPENDENCIES := $(OBJECTS:.o=.d) build/unit_tests.d build/integration_tests.d build/check_coverage.d

.PHONY: all clean test coverage install uninstall

all: $(TARGET)

$(TARGET): $(OBJECTS) | bin
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(UNIT_TARGET): build/unit_tests.o $(LIB_OBJECTS) | bin
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(INTEGRATION_TARGET): build/integration_tests.o | bin
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(COVERAGE_TARGET): build/check_coverage.o | bin
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: src/%.cpp | build
	rm -f $(@:.o=.gcda)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COVERAGE_FLAGS) -MMD -MP -c $(abspath $<) -o $@

build/unit_tests.o: test/unit.cpp | build
	rm -f $(@:.o=.gcda)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(COVERAGE_FLAGS) -MMD -MP -c $(abspath $<) -o $@

build/integration_tests.o: test/integration.cpp test/support.h | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $(abspath $<) -o $@

build/check_coverage.o: test/check_coverage.cpp test/support.h | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $(abspath $<) -o $@

bin build:
	mkdir -p $@

test: $(TARGET) $(UNIT_TARGET) $(INTEGRATION_TARGET)
	$(INTEGRATION_TARGET) $(TARGET) $(UNIT_TARGET)

coverage: $(TARGET) $(UNIT_TARGET) $(INTEGRATION_TARGET) $(COVERAGE_TARGET)
	$(COVERAGE_TARGET) --reset
	$(MAKE) test
	$(COVERAGE_TARGET) --minimum 95

clean:
	rm -rf bin build coverage

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/lab

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/lab

-include $(DEPENDENCIES)
