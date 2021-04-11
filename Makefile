all: xcpuchk

sources = xcpuchk.cc

obj_files = $(patsubst %.cc,%.o,$(sources))

CXXFLAGS += -g -pthread -flto -O2

xcpuchk: $(obj_files)
	$(CXX) -o $@ $^ -pthread
