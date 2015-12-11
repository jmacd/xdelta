package main

import (
	"fmt"
	"io"
	"os"

	"xdelta"
)

const (
	xdataset = "/Users/jmacd/src/testdata"
	xcompare = "/Users/jmacd/src/xdelta3-3.0.10/xdelta3"
	xdelta3 = "/Users/jmacd/src/xdelta-devel/xdelta3/xdelta3"
	seed = 1422253499919909358
)

type Config struct {
	srcbuf_size int64
	window_size int64
	blocksize   int
}

func NewC() Config {
	// TODO make these (and above) flags
	return Config{1<<26, 1<<22, 1<<16}
}

func (c Config) smokeTest(t *xdelta.TestGroup, p xdelta.Program) {
	target := "Hello world!"
	source := "Hello world, nice to meet you!"

	enc, err := t.Exec("encode", p, true, []string{"-e"})
	if err != nil {
		t.Panic(err)
	}
	dec, err := t.Exec("decode", p, true, []string{"-d"})
	if err != nil {
		t.Panic(err)
	}

	encodeout := t.Drain(enc.Stdout, "encode.stdout")
	decodeout := t.Drain(dec.Stdout, "decode.stdout")

	t.Empty(enc.Stderr, "encode")
	t.Empty(dec.Stderr, "decode")

	t.TestWrite("encode.stdin", enc.Stdin, []byte(target))
	t.TestWrite("encode.srcin", enc.Srcin, []byte(source))

	t.TestWrite("decode.stdin", dec.Stdin, <-encodeout)
	t.TestWrite("decode.srcin", dec.Srcin, []byte(source))

	if do := string(<-decodeout); do != target {
		t.Panic(fmt.Errorf("It's not working! %s\n!=\n%s\n", do, target))
	}
	t.Wait(enc, dec)
}

type PairTest struct {
	// Input
	Config
	program xdelta.Program
	source, target string

	// Output
	encoded int64
}

// P is the test program, Q is the reference version.
func (cfg Config) datasetTest(t *xdelta.TestGroup, p, q xdelta.Program) {
	dir, err := os.Open(xdataset)
	if err != nil {
		t.Panic(err)
	}
	dents, err := dir.Readdir(-1)
	if err != nil {
		t.Panic(err)
	}
	paths := make([]string, len(dents))
	var total int64
	for i, d := range dents {
		if !d.Mode().IsRegular() {
			continue
		}
		paths[i] = fmt.Sprint(xdataset, "/", d.Name())
		total += d.Size()
	}
	meansize := total / int64(len(dents))
	for _, in1 := range paths {
		for _, in2 := range paths {
			if in1 == in2 { continue }

			largest := uint(20)
			for ; largest <= 31 && 1<<largest < meansize; largest++ {}

			// 1/8, 1/4, 1/2, and 1/1 of the power-of-2 rounded-up mean size
			for b := largest /* - 3*/; b <= largest; b++ {
				c1 := cfg
				c1.srcbuf_size = 1<<b
				ptest := &PairTest{c1, p, in1, in2, -1}
				ptest.datasetPairTest(t, 1<<b);
				qtest := &PairTest{c1, q, in1, in2, -1}
				qtest.datasetPairTest(t, 1<<b)
			}
		}
	}
}

func (pt *PairTest) datasetPairTest(t *xdelta.TestGroup, meanSize int64) {
	cfg := pt.Config
	eargs := []string{"-e", fmt.Sprint("-B", cfg.srcbuf_size), "-q",
		fmt.Sprint("-W", cfg.window_size), "-s", pt.source, pt.target}
	enc, err := t.Exec("encode", pt.program, false, eargs)
	if err != nil {
		t.Panic(err)
	}

	dargs := []string{"-dc", fmt.Sprint("-B", cfg.srcbuf_size), "-q",
		fmt.Sprint("-W", cfg.window_size), "-s", pt.source}
	dec, err := t.Exec("decode", pt.program, false, dargs)
	if err != nil {
		t.Panic(err)
	}
	tgt_check, err := os.Open(pt.target)
	if err != nil {
		t.Panic(err)
	}
	tgt_info, err := tgt_check.Stat()
	if err != nil {
		t.Panic(err)
	}
	t.Empty(enc.Stderr, "encode")
	t.Empty(dec.Stderr, "decode")
	t.CopyStreams(enc.Stdout, dec.Stdin, &pt.encoded)
	t.CompareStreams(dec.Stdout, tgt_check, tgt_info.Size())

	t.Wait(enc, dec)

	fmt.Println("PairTest", pt, "success")
}

func (cfg Config) offsetTest(t *xdelta.TestGroup, p xdelta.Program, offset, length int64) {
	eargs := []string{"-e", "-0", fmt.Sprint("-B", cfg.srcbuf_size), "-q",
		fmt.Sprint("-W", cfg.window_size)}
	enc, err := t.Exec("encode", p, true, eargs)
	if err != nil {
		t.Panic(err)
	}
	
	dargs := []string{"-d", fmt.Sprint("-B", cfg.srcbuf_size), "-q",
		fmt.Sprint("-W", cfg.window_size)}
	dec, err := t.Exec("decode", p, true, dargs)
	if err != nil {
		t.Panic(err)
	}

	// The pipe used to read the decoder output and compare
	// against the target.
	read, write := io.Pipe()

	t.Empty(enc.Stderr, "encode")
	t.Empty(dec.Stderr, "decode")

	var encoded_size int64
	t.CopyStreams(enc.Stdout, dec.Stdin, &encoded_size)
	t.CompareStreams(dec.Stdout, read, length)

	// The decoder output ("read", above) is compared with the
	// test-provided output ("write", below).  The following
	// generates the input and output twice.
	t.WriteRstreams("encode", seed, offset, length, enc.Srcin, enc.Stdin)
	t.WriteRstreams("decode", seed, offset, length, dec.Srcin, write)
	t.Wait(enc, dec)

	expect := cfg.srcbuf_size - offset
	if float64(encoded_size) < (0.95 * float64(expect)) ||
		float64(encoded_size) > (1.05 * float64(expect)) {
		t.Fail("encoded size should be ~=", expect, ", actual ", encoded_size)
	}
}

func main() {
	r, err := xdelta.NewRunner()
	if err != nil {
		panic(err)
	}
	defer r.Cleanup()

	cfg := NewC()

	prog := xdelta.Program{xdelta3}

	r.RunTest("smoketest", func(t *xdelta.TestGroup) { cfg.smokeTest(t, prog) })

	comp := xdelta.Program{xcompare}

	r.RunTest("dataset", func(t *xdelta.TestGroup) { cfg.datasetTest(t, prog, comp) })

	for i := uint(19); i <= 30; i += 1 {
		// The arguments to offsetTest are offset, source
		// window size, and file size. The source window size
		// is (2 << i) and (in the 3.0x release branch) is
		// limited to 2^31, so the the greatest value of i is
		// 30.
		cfg.srcbuf_size = 2 << i
		r.RunTest(fmt.Sprint("offset", i), func(t *xdelta.TestGroup) {
			cfg.offsetTest(t, prog, 1 << i, 3 << i) })
	}
	
}
