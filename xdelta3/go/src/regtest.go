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
	xdelta3 = "/volume/home/jmacd/src/xdelta-devel/xdelta3/xdelta3"
	seed = 1422253499919909358
)

func smokeTest(r *xdelta.Runner, t *xdelta.TestGroup, p *xdelta.Program) {
	t.Add(1)
	target := "Hello world!"
	source := "Hello world, nice to meet you!"
	
	enc, err := r.Exec(p, true, []string{"-evv"})
	if err != nil {
		t.Panic(err)
	}
	encodeout := t.Drain(enc.Stdout, "encode.stdout")
	t.Empty(enc.Stderr, "encode")

	t.Write("encode.stdin", enc.Stdin, []byte(target))
	t.Write("encode.srcin", enc.Srcin, []byte(source))

	if err := enc.Cmd.Wait(); err != nil {
		t.Panic(err)
	}

	dec, err := r.Exec(p, true, []string{"-dvv"})
	if err != nil {
		t.Panic(err)
	}

	decodeout := t.Drain(dec.Stdout, "decode.stdout")
	t.Empty(dec.Stderr, "decode")

	t.Write("decode.stdin", dec.Stdin, <-encodeout)
	t.Write("decode.srcin", dec.Srcin, []byte(source))
	decoded := string(<-decodeout)
	if err := dec.Cmd.Wait(); err != nil {
		t.Panic(err)
	}
	if decoded != target {
		t.Panic(errors.New("It's not working!!!"))
	}
	t.Done()
	fmt.Println("Smoketest pass")
}

func offsetTest(r *xdelta.Runner, t *xdelta.TestGroup, p *xdelta.Program, bufsize, offset, length int64) {
	t.Add(1)
	eargs := []string{"-e", "-0", fmt.Sprint("-B", bufsize), "-vv", fmt.Sprint("-W", winsize)}
	enc, err := r.Exec(p, true, eargs)
	if err != nil {
		t.Panic(err)
	}
	dargs := []string{"-d", fmt.Sprint("-B", bufsize), "-vv", fmt.Sprint("-W", winsize)}
	dec, err := r.Exec(p, true, dargs)
	if err != nil {
		t.Panic(err)
	}

	read, write := io.Pipe()

	t.Empty(enc.Stderr, "encode")
	t.Empty(dec.Stderr, "decode")

	t.CopyStreams(enc.Stdout, dec.Stdin)
	t.CompareStreams(dec.Stdout, read, length)

	// TODO: seems possible to use one WriteRstreams call to generate
	// the source and target for both encoder and decoder.  Why not?
	xdelta.WriteRstreams(t, seed, offset, length, enc.Srcin, enc.Stdin)
	xdelta.WriteRstreams(t, seed, offset, length, dec.Srcin, write)

	if err := enc.Cmd.Wait(); err != nil {
		t.Panic(err)
	}
	if err := dec.Cmd.Wait(); err != nil {
		t.Panic(err)
	}
	t.Done()
}

func main() {
	r, err := xdelta.NewRunner()
	if err != nil {
		panic(err)
	}
	defer r.Cleanup()

	prog := &xdelta.Program{xdelta3}

	smokeTest(r, xdelta.NewTestGroup(), prog)
	offsetTest(r, xdelta.NewTestGroup(), prog, 4 << 20, 3 << 20, 5 << 20)

	//offsetTest(r, xdelta.NewTestGroup(), prog, 1 << 31, 1 << 32, 1 << 33)
}
