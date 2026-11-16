CXX = g++
TJG_ROOT = ..
FMT_ROOT ?= $(HOME)/App/fmt
FMT_LIB ?= $(FMT_ROOT)/build/libfmtd.a
CXXFLAGS += -std=gnu++23 -Wall -Wextra -Werror

TJG_DEMOS = FmtColorDemo1 FmtColorDemo2 TableDemo OstreamDemo NestedDemo
FMT_DEMOS = FmtColorDemo0 NestedDemo
DEMOS = $(TJG_DEMOS) Cpp23Demo FmtColorDemo0

find_headers = $(wildcard $(1)*.h $(1)*.hpp) \
               $(foreach dir,$(wildcard $(1)*/),$(call find_headers,$(dir)))
TJG_HEADERS := $(call find_headers,$(TJG_ROOT)/include/tjg/)

.PHONY: all run clean
all: $(DEMOS)

$(DEMOS): %: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

$(TJG_DEMOS): CPPFLAGS += -I$(TJG_ROOT)/include
$(TJG_DEMOS): $(TJG_HEADERS)

$(FMT_DEMOS): CPPFLAGS += -I"$(FMT_ROOT)/include"
$(FMT_DEMOS): LDLIBS += "$(FMT_LIB)"
$(FMT_DEMOS): $(FMT_LIB)

run: all
	./FmtColorDemo1
	./FmtColorDemo2
	./TableDemo
	./OstreamDemo
	./Cpp23Demo
	./FmtColorDemo0
	./NestedDemo

clean:
	$(RM) $(DEMOS)
