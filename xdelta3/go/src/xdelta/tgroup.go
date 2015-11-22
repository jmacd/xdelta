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
	g.errChan <- nil
	_ = <- g.errChan
}

func (g *Goroutine) Panic(err error) {
	g.errChan <- err
	_ = <- g.errChan
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
	t.waitChan = nil
	wc := t.waitChan
	t.Unlock()
	_ = <- wc
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

		timeout := make(chan bool, 1)
		go func() {
			time.Sleep(1 * time.Second)
			timeout <- true
		}()
		fmt.Println("Waiting on", runner)
		select {
		case err := <- runner.errChan:
			runner.errChan <- err
			if err != nil {
				fmt.Println("[G]", runner, err)
			} else {
				fmt.Println("[G]", runner, "OK")
			}
		case <- timeout:
			t.Lock()
			t.running = append(t.running, runner)
			t.Unlock()
		}
	}
	wc <- true
}
