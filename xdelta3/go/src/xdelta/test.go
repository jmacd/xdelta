package xdelta

import (
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

type Runner struct {
	Testdir string
}

type Run struct {
	Cmd exec.Cmd
	Srcfile string
	Stdin io.WriteCloser
	Srcin io.WriteCloser
	Stdout io.ReadCloser
	Stderr io.ReadCloser
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

func writeFifo(srcfile string, read io.Reader) {
	fifo, err := os.OpenFile(srcfile, os.O_WRONLY, 0600)
	if err != nil {
		panic(err)
	}
	if _, err := io.Copy(fifo, read); err != nil {
		panic(err)
	}
	if err := fifo.Close(); err != nil {
		panic(err)
	}
}
