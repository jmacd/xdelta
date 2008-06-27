CFLAGS = -g -Wall -I.. -DXD3_DEBUG=1 -DNDEBUG=0

DEPS = ../*.h ../*.c *.cc

TARGETS = xdelta3-regtest

all: $(TARGETS)

xdelta3-regtest: $(DEPS)
	$(CXX) $(CFLAGS) xdelta3-regtest.cc -o xdelta3-regtest

clean:
	rm -f *.exe *.stackdump $(TARGETS)
