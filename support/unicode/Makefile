PROJDIR := $(abspath ../..)

CXXFLAGS += -std=c++23 -Wall -Wextra -Wpedantic -Werror
BUILD_DIR ?= ./build

ifneq ($(filter MSYS%,$(shell uname -s)),)
# MSYS libstdc++ <print> needs stdio declarations hidden in strict C++ mode.
CPPFLAGS += -D_DEFAULT_SOURCE
endif

ifeq ($(OS),Windows_NT)
EXEEXT ?= .exe
endif

PRG1 := GenWideRanges
PRG2 := GenUnicodeTables

BIN1 := $(addprefix $(BUILD_DIR)/,$(addsuffix $(EXEEXT),$(PRG1)))
BIN2 := $(addprefix $(BUILD_DIR)/,$(addsuffix $(EXEEXT),$(PRG2)))

BINARIES := $(BIN1) $(BIN2)

INCLUDE := $(PROJDIR)/include/tjg/bits

.PHONY: all clean
all: $(addprefix $(INCLUDE)/, UnicodeData.hpp EastAsianWidth.hpp)

$(BUILD_DIR)/%$(EXEEXT): %.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) "$<" $(LDLIBS) -o "$@"

$(INCLUDE)/EastAsianWidth.hpp: $(BIN1) EastAsianWidth.txt
	$(BIN1) EastAsianWidth.txt > $@

$(INCLUDE)/UnicodeData.hpp: $(BIN2) UnicodeData.txt
	$(BIN2) UnicodeData.txt > $@

$(BUILD_DIR):
	mkdir -p "$@"

clean:
	$(RM) -- $(BINARIES)
