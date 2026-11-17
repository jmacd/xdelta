---
original_title: ""
date: "2007-06-28T17:00:00.000-07:00"
updated: "2007-06-29T01:28:48.382-07:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2007/06/no-new-versions-posted-since-march.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-1786326876066769626"
---

# Post from 2007-06-28 17:00

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2007-06-28T17:00:00.000-07:00 | [Original post](https://xdelta.blogspot.com/2007/06/no-new-versions-posted-since-march.html)

<div class="blogger-post">
No new versions posted since March, so a few updates. One user sent a MSDOS <code>.bat</code> <a href="http://code.google.com/p/xdelta/issues/detail?id=40">script</a> for xdelta1/xdelta3 command-line compatibility, another sent a <code>perl</code> <a href="http://code.google.com/p/xdelta/issues/detail?id=21">script</a> for recursive directory diff, one user reports good performance for an in-kernel application (<a href="http://xdelta.googlecode.com/svn/trunk/xdelta3/examples/small_page_test.c">sample code</a>), and some <a href="http://code.google.com/p/xdelta/issues/list">feature requests</a>. Given the lack of bug reports, it's about time to take xdelta3 out of beta.<br /><br />Several of you have requested a feature for supporting <em>in-place</em> updates, allowing you to apply deltas without making a copy of the file, which brings me to another bit of user feedback:<ul>Firstly, I’d just like to let you know that we did a little benchmark at work and xdelta3 came out on top from several utilities in terms of both execution speed and final output size.<br /><br />One of those utilities is a 4.5 year old version of RTPatch which we have a license for.<br /><br />One of my work colleagues emailed Pocket Soft about this, and as well as the obvious “you’re using an old version” response, and “open source software has no support” arguments, they also said something along the lines of: “xdelta likely infringes on our patents as well”, implying that using it may be illegal.<br /><br />Being a great supporter and user of open source in my spare time, I am totally against this sort of spreading of F.U.D., as well as the very idea that software is patentable.<br /><br />I’m not sure if my work colleague emailed you as well, but I just thought I’d let you know. :)</ul>And the funny part is, users were saying 5 years ago that <a href="http://xdelta.org/2007/02/re-performance-from-old-joel-on.html">Xdelta 1.x beats RTPatch</a>. :)<br /><br />I have to thank the IETF and previous work in open source (e.g., <a href="http://www.ietf.org/rfc/rfc1950.txt">RFC 1950</a> &ndash; Zlib, <a href="http://www.ietf.org/rfc/rfc3284.txt">RFC 3284</a> &ndash; VCDIFF) for making this possible. <a href="http://zlib.net">Zlib</a> bills itself "A Massively Spiffy Yet Delicately Unobtrusive Compression Library (Also Free, Not to Mention Unencumbered by Patents)", and in fact Zlib inspired Xdelta's API from the start (it's "unobtrusive"). Let's not forget Zlib's other main advantage (it's "unencumbered").  As for the the previous request (in-place updates), interest is strong but patents could become an issue.<br /><br />Multi-threaded encoding/decoding is another frequent request. The idea is that more CPUs can encode/decode faster by running in parallel over interleaved segments of the inputs. That's future work, and probably a lot of it, but I like the idea.<br /><br />Xdelta 3.0q has 11,480 downloads. It's you the user that feeds open source, and thanks for the great feedback!
</div>
