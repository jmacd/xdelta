---
original_title: ""
date: "2008-09-04T21:00:00.000-07:00"
updated: "2008-09-04T22:10:08.094-07:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2008/09/re-googles-open-vcdiff-google-released.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-4531223475170201009"
---

# Post from 2008-09-04 21:00

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2008-09-04T21:00:00.000-07:00 | [Original post](https://xdelta.blogspot.com/2008/09/re-googles-open-vcdiff-google-released.html)

<div class="blogger-post">
Re: <a href="http://google-opensource.blogspot.com/2008/09/open-vcdiff-encoder-and-decoder-for-rfc.html">Google's open-vcdiff</a><br /><br />Google released a new <a href="http://code.google.com/p/open-vcdiff/">open-source library for RFC 3284</a> (VCDIFF) encoding and decoding, designed to support their proposed <a href="http://groups.google.com/group/SDCH">Shared Dictionary Compression over HTTP</a> (SDCH, a.k.a. "Sandwich") protocol. <br /><br />This is great news.  The author and I have had numerous discussions over a couple of features that VCDIFF lacks, and now that we have two open-source implementations we're able to make real progress on the standard.  Both Google's open-vcdiff and Xdelta-3.x implement a extensions to the standard, but I ran a simple interoperability test and things look good.  Run xdelta3 with "-n" to disable checksums and "-A" to disable its application header.<br /><br />A big thanks to Lincoln Smith and Google's contributors. (Disclaimer: I am an employee of Google, but I did not contribute any code to open-vcdiff.)<br /><br />P.S. I'm still debugging the <tt>xdelta3 merge</tt> command, stay tuned to <a href="http://code.google.com/p/xdelta/issues/detail?id=36">issue 36</a>.
</div>
