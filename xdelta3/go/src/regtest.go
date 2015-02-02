package main

import (
	"bufio"
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"os"

	"xdelta"
)

const (
	blocksize = 1<<16
	winsize = 1<<26
	prog = "/Users/jmacd/src/xdelta/xdelta3/build/m64/64size-64off/xdelta3"
	seed = 1422253499919909358
)

func drain(f io.ReadCloser) <-chan []byte {
	c := make(chan []byte)
	go func() {
		if b, err := ioutil.ReadAll(f); err != nil {
			panic(err)
		} else {
			c <- b
		}
	}()
	return c
}

func empty(f io.ReadCloser, desc string) {
	go func() {
		s := bufio.NewScanner(f)
		for s.Scan() {
			os.Stderr.Write([]byte(fmt.Sprint(desc, ": ", s.Text(), "\n")))
		}
		if err := s.Err(); err != nil {
			fmt.Println("error reading input:", err)
		}
	}()
}

func write(f io.WriteCloser, b []byte) {
	if _, err := f.Write(b); err != nil {
		panic(err)
	}
	if err := f.Close(); err != nil {
		panic(err)
	}
}

func smokeTest(r *xdelta.Runner, p *xdelta.Program) {
	target := "Hello world!"
	source := "Hello world, nice to meet you!"
	
	run, err := r.Exec(p, true, []string{"-e"})
	if err != nil {
		panic(err)
	}
	encodeout := drain(run.Stdout)
	empty(run.Stderr, "encode")

	write(run.Stdin, []byte(target))
	write(run.Srcin, []byte(source))

	if err := run.Cmd.Wait(); err != nil {
		panic(err)
	}

	run, err = r.Exec(p, true, []string{"-d"})
	if err != nil {
		panic(err)
	}

	decodeout := drain(run.Stdout)
	empty(run.Stderr, "decode")

	write(run.Stdin, <-encodeout)
	write(run.Srcin, []byte(source))

	if err := run.Cmd.Wait(); err != nil {
		panic(err)
	}

	if string(<-decodeout) != target {
		panic("It's not working!!!")
	}
}

func copyStreams(r io.ReadCloser, w io.WriteCloser) {
	_, err := io.Copy(w, r)
	if err != nil {
		panic(err)
	}
	err = r.Close()
	if err != nil {
		panic(err)
	}
	err = w.Close()
	if err != nil {
		panic(err)
	}
}

func compareStreams(r1 io.ReadCloser, r2 io.ReadCloser, length int64) {
	b1 := make([]byte, blocksize)
	b2 := make([]byte, blocksize)
	var idx int64
	for length > 0 {
		c := blocksize
		if length < blocksize {
			c = int(length)
		}
		if _, err := io.ReadFull(r1, b1[0:c]); err != nil {
			panic(err)
		}
		if _, err := io.ReadFull(r2, b2[0:c]); err != nil {
			panic(err)
		}
		if bytes.Compare(b1[0:c], b2[0:c]) != 0 {
			fmt.Println("B1 is", string(b1[0:c]))
			fmt.Println("B2 is", string(b2[0:c]))			
			panic(fmt.Sprint("Bytes do not compare at ", idx))
		}
		length -= int64(c)
		idx += int64(c)
	}
}

func offsetTest(r *xdelta.Runner, p *xdelta.Program, offset, bufsize, length int64) {
	enc, err := r.Exec(p, true, []string{"-e", "-1", "-n", fmt.Sprint("-B", bufsize), "-vv", fmt.Sprint("-W", winsize)})
	if err != nil {
		panic(err)
	}
	dec, err := r.Exec(p, true, []string{"-d", fmt.Sprint("-B", bufsize), "-vv", fmt.Sprint("-W", winsize)})
	if err != nil {
		panic(err)
	}

	read, write := io.Pipe()

	go copyStreams(enc.Stdout, dec.Stdin)
	go compareStreams(dec.Stdout, read, length)

	empty(enc.Stderr, "encode")
	empty(dec.Stderr, "decode")

	// TODO: seems possible to use one WriteRstreams call to generate
	// the source and target for both encoder and decoder.  Why not?
	xdelta.WriteRstreams(seed, offset, length, enc.Srcin, enc.Stdin)
	xdelta.WriteRstreams(seed, offset, length, dec.Srcin, write)

	if err := enc.Cmd.Wait(); err != nil {
		panic(err)
	}
	if err := dec.Cmd.Wait(); err != nil {
		panic(err)
	}
}

func main() {
	r, err := xdelta.NewRunner()
	if err != nil {
		panic(err)
	}
	defer r.Cleanup()

	prog := &xdelta.Program{prog}

	smokeTest(r, prog)

	offsetTest(r, prog, 1 << 31, 1 << 32, 1 << 33)
}
