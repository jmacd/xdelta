package xdelta

import (
	"io/ioutil"
	"os"
)

type Runner struct {
	Testdir string
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
