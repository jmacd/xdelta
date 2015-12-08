package xdelta

import (
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"os/exec"
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

type Runner struct {
	Testdir string
}

func (r *Run) Wait() error {
	return r.Cmd.Wait()
}

func NewRunner() (*Runner, error) {
	if dir, err := ioutil.TempDir(tmpDir, "xrt"); err != nil {
		return nil, err
	} else {
		return &Runner{dir}, nil
	}
}

func (r *Runner) newTestGroup(name string) (*TestGroup) {
	tg := &TestGroup{Runner: r}
	tg.WaitGroup.Add(1)
	g0 := &Goroutine{tg, name, false}
	tg.running = append(tg.running, g0)
	tg.main = g0
	return tg
}

func (r *Runner) Cleanup() {
	os.RemoveAll(r.Testdir)
}

func (r *Runner) RunTest(name string, f func (t *TestGroup)) {
	t := r.newTestGroup(name)
	var rec interface{}
	defer func() {
		if r := recover(); r != nil {
			fmt.Println("PANIC in ", name, ": ", r)
			rec = r
		} else {
			// Goexit
		}
	}()
	fmt.Println("Testing", name, "...")
	f(t)
	if t.errors == nil && rec == nil {
		fmt.Println("Success:", name)
	} else {
		fmt.Println("FAILED:", name, t.errors, rec)
	}
}
