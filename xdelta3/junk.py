#!/usr/bin/python

bytes = ''

for x in range(0, 250):
  bytes = bytes + ('%c%c%c%c=' % (x, x+1, x+2, x+3))

for x in range(0, 250):
  bytes = bytes + ('%c' % x)
  
print bytes
