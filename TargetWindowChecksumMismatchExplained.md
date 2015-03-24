Explains the "target window checksum mismatch: XD3\_INALID\_INPUT" error message.

# Introduction #

The xdelta3 command-line tool has a couple of constraints which make it tricky to report a precise error when something goes wrong. Because it supports streaming both the source and target inputs, it can't verify a whole-file checksum before beginning its operations. Because the VCDIFF file format is window-oriented, it only detects checksum failures at the level of a specific window. This often means that the decoder will finish partially decoding an input before recognizing that it is using the wrong source input.

**This error message typically means you have the incorrect source file.**  Please use sha1sum or another strong checksum to verify that the source file is correct.

# Details #

It would require two passes over the data in both encoding and decoding to properly encode the checksum as part of the patch file and then verify it. This is not possible for streaming inputs.

It would be possible to verify the source-file checksum in parallel with encoding or decoding, however this would require new file-format support to include metadata _at the end of the delta encoding_.  There is no similar feature in the VCDIFF encoding, only the ability to place metadata in the header.  Since we can't read the entire source file before writing the header, this is not done. It would add unnecessary slowness and code complexity, too, so this feature -- the ability to detect incorrect source file without applying the delta -- is not implemented.

The target-window checksum is a catch-all for other kinds of error, too.  An incorrect encoding (a bug, if it exists), hardware failure and internal errors can also cause it too. However, using the incorrect source file is the only "normal" cause of this error, so you should check that first.