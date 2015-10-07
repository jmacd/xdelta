package xdelta


import (
	"io"
	"math/rand"
)

const (
	blocksize = 16380
)

func WriteRstreams(t *TestGroup, seed, offset, len int64,
	first, second io.WriteCloser) {
	go writeOne(t, seed, 0, len, first)
	go writeOne(t, seed, offset, len, second)
}

func writeOne(t *TestGroup, seed, offset, len int64, stream io.WriteCloser) error {
	t.WaitGroup.Add(1)
	if offset != 0 {
		// Fill with other random data until the offset
		if err := writeRand(rand.New(rand.NewSource(^seed)), offset, stream); err != nil {
			return err
		}
	}
	if err := writeRand(rand.New(rand.NewSource(seed)),
		len - offset, stream); err != nil {
		return err
	}
	t.WaitGroup.Done()
	return stream.Close()
}

func writeRand(r *rand.Rand, len int64, s io.Writer) error {
	blk := make([]byte, blocksize)
	for len > 0 {
		fillRand(r, blk)
		c := blocksize
		if len < blocksize {
			c = int(len)
		}
		if _, err := s.Write(blk[0:c]); err != nil {
			return err
		}
		len -= int64(c)
	}
	return nil
}

func fillRand(r *rand.Rand, blk []byte) {
	for p := 0; p < blocksize; {
		v := r.Int63()
		for i := 7; i != 0; i-- {
			blk[p] = byte(v)
			p++
			v >>= 8
		}
	}
}
