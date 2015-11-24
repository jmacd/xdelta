package xdelta

import (
	"bufio"
	"bytes"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"os/exec"
	"path"
	"sync/atomic"

	"golang.org/x/sys/unix"
)

var (
	tmpDir = "/tmp"
	srcSeq int64
)

type Program struct {
	Path string
}

type Run struct {
	Cmd exec.Cmd
	Srcfile string
	Stdin io.WriteCloser
	Srcin io.WriteCloser
	Stdout io.ReadCloser
	Stderr io.ReadCloser
}

func (t *TestGroup) Drain(f io.ReadCloser, desc string) <-chan []byte {
	c := make(chan []byte)
	t.Go(desc, func(g Goroutine) {
		if b, err := ioutil.ReadAll(f); err != nil {
			fmt.Println("Drain", err)
			g.Panic(err)
		} else {
			c <- b
		}
		g.OK()
	})
	return c
}

func (t *TestGroup) Empty(f io.ReadCloser, desc string) {
	t.Go(desc, func (g Goroutine) {
		g.OK()
		s := bufio.NewScanner(f)
		for s.Scan() {
			os.Stderr.Write([]byte(fmt.Sprint(desc, ": ", s.Text(), "\n")))
		}
		err := s.Err()
		f.Close()
		if err != nil {
			fmt.Println("Empty", desc, err)
			g.Panic(err)
		}
	})
}

func TestWrite(what string, f io.WriteCloser, b []byte) error {
	if _, err := f.Write(b); err != nil {
		fmt.Println("Write", err)
		return errors.New(fmt.Sprint(what, ":", err))
	}
	if err := f.Close(); err != nil {
		fmt.Println("Close", err)
		return errors.New(fmt.Sprint(what, ":", err))
	}
	return nil
}

func (t *TestGroup) CopyStreams(r io.ReadCloser, w io.WriteCloser) {
	t.Go("copy", func(g Goroutine) {
		_, err := io.Copy(w, r)
		if err != nil {
			fmt.Println("CopyS", err)
			g.Panic(err)
		}
		err = r.Close()
		if err != nil {
			fmt.Println("CloseS1", err)
			g.Panic(err)
		}
		err = w.Close()
		if err != nil {
			fmt.Println("CloseS2", err)
			g.Panic(err)
		}
		g.OK()
	})
}

func (t *TestGroup) CompareStreams(r1 io.ReadCloser, r2 io.ReadCloser, length int64) {
	t.Go("compare", func(g Goroutine) {
		b1 := make([]byte, blocksize)
		b2 := make([]byte, blocksize)
		var idx int64
		for length > 0 {
			c := blocksize
			if length < blocksize {
				c = int(length)
			}
			if _, err := io.ReadFull(r1, b1[0:c]); err != nil {
				fmt.Println("ReadFull1", err)
				g.Panic(err)
			}
			if _, err := io.ReadFull(r2, b2[0:c]); err != nil {
				fmt.Println("ReadFull2", err)
				g.Panic(err)
			}
			if bytes.Compare(b1[0:c], b2[0:c]) != 0 {
				fmt.Println("B1 is", string(b1[0:c]))
				fmt.Println("B2 is", string(b2[0:c]))			
				g.Panic(errors.New(fmt.Sprint("Bytes do not compare at ", idx)))
			}
			length -= int64(c)
			idx += int64(c)
		}
		g.OK()
	})
}

func (t *TestGroup) Exec(desc string, p *Program, srcfifo bool, flags []string) (*Run, error) {
	var err error
	run := &Run{}
	args := []string{p.Path}
	if srcfifo {
		num := atomic.AddInt64(&srcSeq, 1)
		run.Srcfile = path.Join(t.Runner.Testdir, fmt.Sprint("source", num))
		if err = unix.Mkfifo(run.Srcfile, 0600); err != nil {
			return nil, err
		}
		// Because OpenFile blocks on the Fifo until the reader
		// arrives, a pipe to defer open
		read, write := io.Pipe()
		run.Srcin = write

		go writeFifo(run.Srcfile, read)
		args = append(args, "-s")
		args = append(args, run.Srcfile)
	}
	if run.Stdin, err = run.Cmd.StdinPipe(); err != nil {
		return nil, err
	}
	if run.Stdout, err = run.Cmd.StdoutPipe(); err != nil {
		return nil, err
	}
	if run.Stderr, err = run.Cmd.StderrPipe(); err != nil {
		return nil, err
	}

	run.Cmd.Path = p.Path
	run.Cmd.Args = append(args, flags...)
	run.Cmd.Dir = t.Runner.Testdir

	if serr := run.Cmd.Start(); serr != nil {
		return nil, serr
	}
	t.Go("exec-wait:" + desc, func (g Goroutine) {
		if err := run.Cmd.Wait(); err != nil {
			g.Panic(err)
		}
		g.OK()
	})
	return run, nil
}

func writeFifo(srcfile string, read io.Reader) error {
	fifo, err := os.OpenFile(srcfile, os.O_WRONLY, 0600)
	if err != nil {
		fifo.Close()
		return err
	}
	if _, err := io.Copy(fifo, read); err != nil {
		fifo.Close()
		return err
	}
	return fifo.Close()
}
