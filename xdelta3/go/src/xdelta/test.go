package xdelta

import (
	"io"
	"io/ioutil"
	"os"
	"os/exec"
	"path"
	"golang.org/x/sys/unix"
)

var (
	tmpDir = "/tmp"
)

type Program struct {
	Path string
}

type Run struct {
	Cmd exec.Cmd
	Testdir string
	Srcfile string
	Stdin io.WriteCloser
	Srcin io.WriteCloser
	Stdout io.ReadCloser
	Stderr io.ReadCloser
}	

func (b *Program) Exec(srcfifo bool, flags []string) (*Run, error) {
	var err error
	run := &Run{}
	if run.Testdir, err = ioutil.TempDir(tmpDir, "xrt"); err != nil {
		return nil, err
	}
	args := []string{b.Path}
	if srcfifo {
		run.Srcfile = path.Join(run.Testdir, "source")
		if err = unix.Mkfifo(run.Srcfile, 0600); err != nil {
			return nil, err
		}
		// Because OpenFile blocks on the Fifo until the reader
		// arrives, a pipe to defer open
		read, write := io.Pipe()
		run.Srcin = write

		go func() {
			fifo, err := os.OpenFile(run.Srcfile, os.O_WRONLY, 0600)
			if err != nil {
				panic(err)
			}
			if _, err := io.Copy(fifo, read); err != nil {
				panic(err)
			}
			if err := fifo.Close(); err != nil {
				panic(err)
			}
		}()
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

	run.Cmd.Path = b.Path
	run.Cmd.Args = append(args, flags...)
	run.Cmd.Dir = run.Testdir
	run.Cmd.Start()

	return run, nil
}
