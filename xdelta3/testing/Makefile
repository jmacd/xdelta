CFLAGS = -g -Wall -I.. -DXD3_DEBUG=1
#CFLAGS = -g -Wall -I.. -DXD3_DEBUG=2
#CFLAGS = -O2 -Wall -I.. -DXD3_DEBUG=0 -DNDEBUG=1

DEPS = ../*.h ../*.c *.cc *.h

TARGETS = xdelta3-regtest

all: $(TARGETS)

xdelta3-regtest: $(DEPS)
	$(CXX) $(CFLAGS) regtest.cc -o xdelta3-regtest

clean:
	rm -f *.exe *.stackdump $(TARGETS)
