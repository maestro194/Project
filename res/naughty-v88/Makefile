COMMON_OBJ = read.o common.o MurmurHash3.o
CXXFLAGS = -g -Wall
CXXFLAGS += -O3
CXXFLAGS += -DNDEBUG
ALLTAGET=naughty naughty_test

all: $(ALLTAGET)

naughty: main.cc naughty.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ main.cc naughty.o $(COMMON_OBJ)
naughty_test: naughty_test.cc naughty.o $(COMMON_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ naughty_test.cc naughty.o $(COMMON_OBJ)

clean:
	rm -f $(ALLTAGET) $(COMMON_OBJ) naughty.o

depend:
	$(CXX) -MM $(CXXFLAGS) $(DEFINE) *.cc \
	    | sed -e 's,/usr/[^ ]*,,g' -e '/^ *\\$$/d' > .depend
include .depend
