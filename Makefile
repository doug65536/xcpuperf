all: xcpuchk

sources = xcpuchk.cc

obj_files = $(patsubst %.cc,%.o,$(sources))

CXXFLAGS += -g -pthread -O2
LINKFLAGS =

xcpuchk: $(obj_files)
	$(CXX) $(LINKFLAGS) -o $@ $^ -pthread
