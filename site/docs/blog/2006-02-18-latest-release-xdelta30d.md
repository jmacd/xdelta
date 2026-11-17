---
original_title: ""
date: "2006-02-18T17:00:00.000-08:00"
updated: "2006-02-18T19:41:17.733-08:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2006/02/latest-release-xdelta30d.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-114031333704556904"
---

# Post from 2006-02-18 17:00

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2006-02-18T17:00:00.000-08:00 | [Original post](https://xdelta.blogspot.com/2006/02/latest-release-xdelta30d.html)

<div class="blogger-post">
Latest release: <a href="http://xdelta.org/xdelta30d.tar.gz">xdelta30d.tar.gz</a> fixes a bug sent in by at least three users. Thanks for great reporting. The bug was an assertion failure during encode, so it's one you would have noticed, had it happened to you.<br /><br />Several of you have sent patches for building on Windows, which I appreciate. This version should at least be easier to build (e.g., rename a field from <tt>near</tt> to <tt>near_array</tt>, add <tt>"b"</tt> to the <tt>fopen()</tt> mode, eliminate unnecessary <tt>sys/</tt> includes, ...).<br /><br />The author of <a href="http://kdiff3.sourceforge.net/">kdiff3</a> mailed, asking for help visualizing binary diffs. We'll get that working, but in the meanwhile there is a way to print the contents of a VCDIFF input, <tt>xdelta3 printdelta INPUT</tt>. It prints the VCDIFF header, followed by the instruction list. For example:<pre><br />  Offset Code Type1 Size1 @Addr1 + Type2 Size2 @Addr2<br />  000000 019  CPY_0 59357 @0     <br />  059357 013  ADD    12        <br />  059369 037  CPY_1   5 @120794<br />  059374 019  CPY_0 2066 @59370 <br /></pre>
</div>
