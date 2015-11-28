package xdelta

import (
	"fmt"
	"io"
	"sync"
	"time"
)

type TestGroup struct {
	*Runner
	sync.Mutex
	running []Goroutine
	errors []error
	waitChan <-chan bool
}

type Goroutine struct {
	name string
	errChan chan error
}

func NewTestGroup(r *Runner) (*TestGroup, Goroutine) {
	g := Goroutine{"main", make(chan error)}
	wc := make(chan bool)
	tg := &TestGroup{Runner: r, running: []Goroutine{g}, waitChan: wc}
	go waitAll(tg, wc)
	return tg, g
}

func (g *Goroutine) String() string {
	return fmt.Sprint("[", g.name, "]")
}

func (g *Goroutine) OK() {
	if g.errChan != nil {
		g.errChan <- nil
		_ = <- g.errChan
		g.errChan = nil
	}
}

func (g *Goroutine) Panic(err error) {
	fmt.Println(g, err)
	if g.errChan != nil {
		g.errChan <- err
		_ = <- g.errChan
	}
	select {}
}

func (t *TestGroup) Go(name string, f func(Goroutine)) Goroutine {
	g := Goroutine{name, make(chan error)}
	t.Lock()
	t.running = append(t.running, g)
	t.Unlock()
	go f(g)
	return g
}

func (t *TestGroup) Wait(self Goroutine, procs... *Run) {
	self.OK()
	t.Lock()
	wc := t.waitChan
	t.waitChan = nil
	t.Unlock()
	_ = <- wc
	t.Lock()
	errs := t.errors
	t.Unlock()
	for _, p := range procs {
		if err := p.Wait(); err != nil {
			errs = append(errs, err)
		}
	}
	if len(errs) != 0 {
		for _, err := range errs {
			fmt.Println(err)
		}
		panic(fmt.Sprint(len(errs), " errors"))
	}
}

func waitAll(t *TestGroup, wc chan bool) {
	for {
		t.Lock()
		if len(t.running) == 0 {
			t.Unlock()
			break
		}
		runner := t.running[0]
		t.running = t.running[1:]
		t.Unlock()

		timeout := time.After(time.Second)

		select {
		case err := <- runner.errChan:
			runner.errChan <- err
			if err != nil {
				t.Lock()
				t.errors = append(t.errors, err)
				t.Unlock()
			}
		case <- timeout:
			t.Lock()
			t.running = append(t.running, runner)
			t.Unlock()
		}
	}
	wc <- true
}

type dualWriter struct {
	e, d chan []byte
}

func (d *dualWriter) Write(p []byte) (int, error) {
	if len(p) != 0 {
		d.e <- p
		d.d <- p
	}
	return len(p), nil
}

func (d *dualWriter) Close() error {
	d.e <- nil
	d.d <- nil
	_ = <- d.e
	_ = <- d.d
	return nil
}

func newWriter(c chan []byte, a io.WriteCloser) func (Goroutine) {
	return func (g Goroutine) {
		for {
			d := <- c
			if d == nil {
				if err := a.Close(); err != nil {
					g.Panic(err)
				}
				c <- nil
				g.OK()
				return
			}
			if num, err := a.Write(d); err != nil {
				g.Panic(err)
			} else if num != len(d) {
				g.Panic(fmt.Errorf("Invalid write: %v != %v", num, len(d)))
			}
		}
	}
}

func (t *TestGroup) NewDualWriter(a1, a2 io.WriteCloser) io.WriteCloser {
	c1 := make(chan []byte)
	c2 := make(chan []byte)
	r := &dualWriter{c1, c2}
	t.Go("writer0", newWriter(c1, a1))
	t.Go("writer1", newWriter(c2, a2))
	return r
}
