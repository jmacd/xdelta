package xdelta


import (
	"io"
	"fmt"
	"math/rand"
)

const (
	blocksize = 1<<17
)

func (t *TestGroup) WriteRstreams(desc string, seed, offset, len int64,
	src, tgt io.WriteCloser) {
	t.Go("src-write:"+desc, func (g Goroutine) {
		writeOne(g, seed, 0, len, src, false)
	})
	t.Go("tgt-write:"+desc, func (g Goroutine) {
		writeOne(g, seed, offset, len, tgt, true)
	})
}

func writeOne(g Goroutine, seed, offset, len int64, stream io.WriteCloser, readall bool) {
	if !readall {
		// Allow the source-read to fail or block until the process terminates.
		// This behavior is reserved for the decoder, which is not required to
		// read the entire source.
		g.OK()
	}
	if offset != 0 {
		// Fill with other random data until the offset
		fmt.Println(g, "pre-offset case", offset)
		if err := writeRand(g, rand.New(rand.NewSource(^seed)), offset, stream); err != nil {
			g.Panic(err)
		}
	}
	fmt.Println(g, "offset case", len - offset)
	if err := writeRand(g, rand.New(rand.NewSource(seed)),
		len - offset, stream); err != nil {
		g.Panic(err)
	}
	fmt.Println(g, "closing", len)
	if err := stream.Close(); err != nil {
		g.Panic(err)
	}
	g.OK()
}

func writeRand(g Goroutine, r *rand.Rand, len int64, s io.Writer) error {
	blk := make([]byte, blocksize)
	fmt.Println(g, "rstream", len)
	for len > 0 {
		fillRand(r, blk)
		c := blocksize
		if len < blocksize {
			c = int(len)
		}
		fmt.Println(g, "writing...", c, s)
		if _, err := s.Write(blk[0:c]); err != nil {
			return err
		}
		fmt.Println(g, "...written", c)
		len -= int64(c)
	}
	return nil
}

func fillRand(r *rand.Rand, blk []byte) {
	for p := 0; p < len(blk); {
		v := r.Int63()
		for i := 7; i != 0 && p < len(blk); i-- {
			blk[p] = byte(v)
			p++
			v >>= 8
		}
	}
}
