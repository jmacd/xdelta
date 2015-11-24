package xdelta

import (
	"fmt"
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
	fmt.Print("[", g.name, "] ", err, "\n")
	if g.errChan != nil {
		g.errChan <- err
		_ = <- g.errChan
	}
	select {}
}

func (t *TestGroup) Go(name string, f func(Goroutine)) {
	g := Goroutine{name, make(chan error)}
	t.Lock()
	t.running = append(t.running, g)
	t.Unlock()
	go f(g)
}

func (t *TestGroup) Wait(g Goroutine) {
	g.OK()
	t.Lock()
	wc := t.waitChan
	t.waitChan = nil
	t.Unlock()
	_ = <- wc
	t.Lock()
	errs := t.errors
	t.Unlock()
	if len(errs) != 0 {
		panic(fmt.Sprintln(len(errs), "errors in test"))
	}
}

func waitAll(t *TestGroup, wc chan bool) {
	for {
		t.Lock()
		// for _, x := range t.running {
		// 	fmt.Println("RUNNING", x.name)
		// }
		if len(t.running) == 0 {
			t.Unlock()
			break
		}
		runner := t.running[0]
		t.running = t.running[1:]
		t.Unlock()

		timeout := time.After(time.Second)
		// fmt.Println("Waiting on", runner)
		select {
		case err := <- runner.errChan:
			runner.errChan <- err
			if err != nil {
				// fmt.Println("[G]", runner, err)
				t.Lock()
				t.errors = append(t.errors, err)
				t.Unlock()
			} else {
				// fmt.Println("[G]", runner, "OK")
			}
		case <- timeout:
			t.Lock()
			t.running = append(t.running, runner)
			t.Unlock()
		}
	}
	wc <- true
}
