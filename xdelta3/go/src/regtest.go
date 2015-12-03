package main

import (
	"errors"
	"fmt"
	"io"

	"xdelta"
)

const (
	blocksize = 1<<16
	winsize = 1<<22
	xdelta3 = "/Users/jmacd/src/xdelta-devel/xdelta3/xdelta3"
	seed = 1422253499919909358
)

func smokeTest(r *xdelta.Runner, p *xdelta.Program) {
	t, g := xdelta.NewTestGroup(r)
	target := "Hello world!"
	source := "Hello world, nice to meet you!"

	enc, err := t.Exec("encode", p, true, []string{"-evv"})
	if err != nil {
		g.Panic(err)
	}
	encodeout := t.Drain(enc.Stdout, "encode.stdout")
	t.Empty(enc.Stderr, "encode")

	if err := xdelta.TestWrite("encode.stdin", enc.Stdin, []byte(target)); err != nil {
		g.Panic(err)
	}
	if err := xdelta.TestWrite("encode.srcin", enc.Srcin, []byte(source)); err != nil {
		g.Panic(err)
	}

	dec, err := t.Exec("decode", p, true, []string{"-dvv"})
	if err != nil {
		g.Panic(err)
	}

	decodeout := t.Drain(dec.Stdout, "decode.stdout")
	t.Empty(dec.Stderr, "decode")

	if err := xdelta.TestWrite("decode.stdin", dec.Stdin, <-encodeout); err != nil {
		g.Panic(err)
	}
	if err := xdelta.TestWrite("decode.srcin", dec.Srcin, []byte(source)); err != nil {
		g.Panic(err)
	}
	decoded := string(<-decodeout)
	if decoded != target {
		g.Panic(errors.New("It's not working!!!"))
	}
	t.Wait(g, enc, dec)
}

func offsetTest(r *xdelta.Runner, p *xdelta.Program, bufsize, offset, length int64) {
	// Note there is a strong potential to deadlock or fail due to
	// a broken test in this test for several reasons:
	// (a) decoder is not required to read the entire source file
	// (b) decoder defers open to source file until first window received
	// (c) open on a fifo blocks until a reader opens
	// (d) sub-process Wait can invalidate busy file descriptors
	t, g := xdelta.NewTestGroup(r)
	eargs := []string{"-e", "-0", fmt.Sprint("-B", bufsize), "-vv", fmt.Sprint("-W", winsize)}
	enc, err := t.Exec("encode", p, true, eargs)
	if err != nil {
		g.Panic(err)
	}
	
	dargs := []string{"-d", fmt.Sprint("-B", bufsize), "-vv", fmt.Sprint("-W", winsize)}
	dec, err := t.Exec("decode", p, true, dargs)
	if err != nil {
		g.Panic(err)
	}

	read, write := io.Pipe()

	t.Empty(enc.Stderr, "encode")
	t.Empty(dec.Stderr, "decode")

	t.CopyStreams(enc.Stdout, dec.Stdin)
	t.CompareStreams(dec.Stdout, read, length)

	// The decoder output ("read", above) is compared with the
	// test-provided output ("write", below).  The following
	// generates the input and output twice.
	t.WriteRstreams("encode", seed, offset, length, enc.Srcin, enc.Stdin)
	t.WriteRstreams("decode", seed, offset, length, dec.Srcin, write)
	t.Wait(g, enc, dec)
}

func main() {
	r, err := xdelta.NewRunner()
	if err != nil {
		panic(err)
	}
	defer r.Cleanup()

	prog := &xdelta.Program{xdelta3}

	smokeTest(r, prog)
	fmt.Println("Smoke-test pass")

	offsetTest(r, prog, 4 << 20, 3 << 20, 5 << 20)
	fmt.Println("Offset-test pass")

	//offsetTest(r, xdelta.NewTestGroup(), prog, 1 << 31, 1 << 32, 1 << 33)
}
