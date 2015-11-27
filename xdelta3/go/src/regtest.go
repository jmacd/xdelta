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
	fmt.Println("Smoketest pass")
}

func offsetTest(r *xdelta.Runner, p *xdelta.Program, bufsize, offset, length int64) {
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

	xdelta.WriteRstreams(t, "encode", seed, offset, length, enc.Srcin, enc.Stdin)
	xdelta.WriteRstreams(t, "decode", seed, offset, length, dec.Srcin, write)
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
	for {
		offsetTest(r, prog, 4 << 20, 3 << 20, 5 << 20)
	}

	//offsetTest(r, xdelta.NewTestGroup(), prog, 1 << 31, 1 << 32, 1 << 33)
}
