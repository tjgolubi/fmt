CXX = g++
FMT_DIR = ..
CPPFLAGS += -I$(FMT_DIR)/include -DTJG_COLOR_COMPACT_SGR=1
CXXFLAGS += -std=c++23 -Wall -Wextra -Werror

DEMOS = FmtColorDemo2 TableDemo

.PHONY: all run clean
all: $(DEMOS)

$(DEMOS): %: %.cpp $(wildcard $(FMT_DIR)/include/fmt/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

run: all
	./FmtColorDemo2
	./TableDemo

clean:
	$(RM) $(DEMOS)
