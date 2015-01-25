package main

import (
	"io"
	"io/ioutil"
	"xdelta"
)

const (
	prog = "/Users/jmacd/src/xdelta/xdelta3/xdelta3"
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

func empty(f io.ReadCloser) {
	go func() {
		if _, err := ioutil.ReadAll(f); err != nil {
			panic(err)
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

func main() {
	x := xdelta.Program{prog}
	smokeTest(x)
}

func smokeTest(p xdelta.Program) {
	target := "Hello world!"
	source := "Hello world, nice to meet you!"
	
	run, err := p.Exec(true, []string{"-e"})
	if err != nil {
		panic(err)
	}
	encodeout := drain(run.Stdout)
	empty(run.Stderr)

	write(run.Stdin, []byte(target))
	write(run.Srcin, []byte(source))

	if err := run.Cmd.Wait(); err != nil {
		panic(err)
	}

	run, err = p.Exec(true, []string{"-d"})
	if err != nil {
		panic(err)
	}

	decodeout := drain(run.Stdout)
	empty(run.Stderr)

	write(run.Stdin, <-encodeout)
	write(run.Srcin, []byte(source))

	if err := run.Cmd.Wait(); err != nil {
		panic(err)
	}

	if string(<-decodeout) != target {
		panic("It's not working!!!")
	}
}
