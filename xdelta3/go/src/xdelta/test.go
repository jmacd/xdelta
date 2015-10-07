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
	"sync"

	"golang.org/x/sys/unix"
)

var (
	tmpDir = "/tmp"
	srcSeq int64
)

type Program struct {
	Path string
}

type Runner struct {
	Testdir string
}

type TestGroup struct {
	sync.WaitGroup
}

type Run struct {
	Cmd exec.Cmd
	Srcfile string
	Stdin io.WriteCloser
	Srcin io.WriteCloser
	Stdout io.ReadCloser
	Stderr io.ReadCloser
}

func (t *TestGroup) Panic(err error) {
	t.WaitGroup.Done()  // For the caller
	t.WaitGroup.Wait()
	panic(err)
}

func NewTestGroup() *TestGroup {
	return &TestGroup{}
}

func (t *TestGroup) Drain(f io.ReadCloser) <-chan []byte {
	c := make(chan []byte)
	go func() {
		t.WaitGroup.Add(1)
		if b, err := ioutil.ReadAll(f); err != nil {
			t.Panic(err)
		} else {
			c <- b
		}
		t.WaitGroup.Done()
	}()
	return c
}

func (t *TestGroup) Empty(f io.ReadCloser, desc string) {
	go func() {
		t.WaitGroup.Add(1)
		s := bufio.NewScanner(f)
		for s.Scan() {
			os.Stderr.Write([]byte(fmt.Sprint(desc, ": ", s.Text(), "\n")))
		}
		if err := s.Err(); err != nil {
			t.Panic(err)
		}
		t.WaitGroup.Done()
	}()
}

func (t *TestGroup) Write(what string, f io.WriteCloser, b []byte) {
	if _, err := f.Write(b); err != nil {
		t.Panic(errors.New(fmt.Sprint(what, ":", err)))
	}
	if err := f.Close(); err != nil {
		t.Panic(errors.New(fmt.Sprint(what, ":", err)))
	}
}

func (t *TestGroup) CopyStreams(r io.ReadCloser, w io.WriteCloser) {
	t.Add(1)
	_, err := io.Copy(w, r)
	if err != nil {
		t.Panic(err)
	}
	err = r.Close()
	if err != nil {
		t.Panic(err)
	}
	err = w.Close()
	if err != nil {
		t.Panic(err)
	}
	t.Done()
}

func (t *TestGroup) CompareStreams(r1 io.ReadCloser, r2 io.ReadCloser, length int64) {
	t.Add(1)
	b1 := make([]byte, blocksize)
	b2 := make([]byte, blocksize)
	var idx int64
	for length > 0 {
		c := blocksize
		if length < blocksize {
			c = int(length)
		}
		if _, err := io.ReadFull(r1, b1[0:c]); err != nil {
			t.Panic(err)
		}
		if _, err := io.ReadFull(r2, b2[0:c]); err != nil {
			t.Panic(err)
		}
		if bytes.Compare(b1[0:c], b2[0:c]) != 0 {
			fmt.Println("B1 is", string(b1[0:c]))
			fmt.Println("B2 is", string(b2[0:c]))			
			t.Panic(errors.New(fmt.Sprint("Bytes do not compare at ", idx)))
		}
		length -= int64(c)
		idx += int64(c)
	}
	t.Done()
}

func NewRunner() (*Runner, error) {
	if dir, err := ioutil.TempDir(tmpDir, "xrt"); err != nil {
		return nil, err
	} else {
		return &Runner{dir}, nil
	}
}

func (r *Runner) Cleanup() {
	os.RemoveAll(r.Testdir)
}

func (r *Runner) Exec(p *Program, srcfifo bool, flags []string) (*Run, error) {
	var err error
	run := &Run{}
	args := []string{p.Path}
	if srcfifo {
		num := atomic.AddInt64(&srcSeq, 1)
		run.Srcfile = path.Join(r.Testdir, fmt.Sprint("source", num))
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
	run.Cmd.Dir = r.Testdir
	run.Cmd.Start()

	return run, nil
}

func writeFifo(srcfile string, read io.Reader) error {
	fifo, err := os.OpenFile(srcfile, os.O_WRONLY, 0600)
	if err != nil {
		return err
	}
	if _, err := io.Copy(fifo, read); err != nil {
		return err
	}
	if err := fifo.Close(); err != nil {
		return err
	}
	return nil
}
