package main

import (
	"errors"
	"fmt"
	"io"

	"xdelta"
)

const (
	blocksize = 1<<16
	winsize = 1<<26
	xdelta3 = "/volume/home/jmacd/src/xdelta-64bithash/xdelta3/build/x86_64-pc-linux-gnu-m64/usize64/xoff64/xdelta3"
	seed = 1422253499919909358
)

func smokeTest(r *xdelta.Runner, t *xdelta.TestGroup, p *xdelta.Program) {
	t.Add(1)
	target := "Hello world!"
	source := "Hello world, nice to meet you!"
	
	run, err := r.Exec(p, true, []string{"-e"})
	if err != nil {
		t.Panic(err)
	}
	encodeout := t.Drain(run.Stdout)
	t.Empty(run.Stderr, "encode")

	t.Write("encode.stdin", run.Stdin, []byte(target))
	t.Write("encode.stdout", run.Srcin, []byte(source))

	if err := run.Cmd.Wait(); err != nil {
		t.Panic(err)
	}

	run, err = r.Exec(p, true, []string{"-d"})
	if err != nil {
		t.Panic(err)
	}

	decodeout := t.Drain(run.Stdout)
	t.Empty(run.Stderr, "decode")

	t.Write("decode.stdin", run.Stdin, <-encodeout)
	t.Write("decode.stdout", run.Srcin, []byte(source))

	if err := run.Cmd.Wait(); err != nil {
		t.Panic(err)
	}

	if string(<-decodeout) != target {
		t.Panic(errors.New("It's not working!!!"))
	}
	t.Done()
}

func offsetTest(r *xdelta.Runner, t *xdelta.TestGroup, p *xdelta.Program, offset, bufsize, length int64) {
	t.Add(1)
	eargs := []string{"-e", "-1", "-n", fmt.Sprint("-B", bufsize), "-vv", fmt.Sprint("-W", winsize)}
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

	t.CopyStreams(enc.Stdout, dec.Stdin)
	t.CompareStreams(dec.Stdout, read, length)

	t.Empty(enc.Stderr, "encode")
	t.Empty(dec.Stderr, "decode")

	// TODO: seems possible to use one WriteRstreams call to generate
	// the source and target for both encoder and decoder.  Why not?
	xdelta.WriteRstreams(t, seed, offset, length, enc.Srcin, enc. Stdin)
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

	offsetTest(r, xdelta.NewTestGroup(), prog, 1 << 8, 1 << 9, 1 << 10)

	//offsetTest(r, prog, 1 << 31, 1 << 32, 1 << 33)
}
