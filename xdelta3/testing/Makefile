CFLAGS = -g -Wall -Wconversion -Wsign-compare -Wextra \
	       -Wno-unused-parameter -I.. \
	      -DGENERIC_ENCODE_TABLES=1 \
	      -DSECONDARY_DJW=1 \
	      -DSECONDARY_FGK=1 \
	      -DXD3_USE_LARGEFILE64=1

DEPS = ../*.h ../*.c *.cc *.h

TARGETS = xdelta3-regtest xdelta3-regtest2

all: $(TARGETS)

xdelta3-regtest: $(DEPS)
	$(CXX) $(CFLAGS) -DXD3_DEBUG=1 regtest.cc -o xdelta3-regtest

xdelta3-regtest2: $(DEPS)
	$(CXX) $(CFLAGS) -DXD3_DEBUG=2 regtest.cc -o xdelta3-regtest2

clean:
	rm -f *.exe *.stackdump $(TARGETS)
