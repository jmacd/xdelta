---
original_title: ""
date: "2008-04-21T23:59:00.000-07:00"
updated: "2008-04-21T21:28:24.064-07:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2008/04/update-for-april-2008-i-started-making.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-1441733357676578995"
---

# Post from 2008-04-21 23:59

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2008-04-21T23:59:00.000-07:00 | [Original post](https://xdelta.blogspot.com/2008/04/update-for-april-2008-i-started-making.html)

<div class="blogger-post">
Update for April 2008:<br /><br />I started making beta releases for Xdelta-3.x four years ago.  There were a lot of bugs then.  Now the bug reports are dwindling, so much so that I've had a chance to work on new features, such as one requested by <a href="http://code.google.com/p/xdelta/issues/detail?id=36">issue 36</a>. Announcing the <tt>xdelta3 merge</tt> command.  Syntax:<pre>xdelta3 merge -m input0 [-m input1 [-m input2 ... ]] inputN output</pre>This command allows you to combine a sequence of deltas, producing a single delta that combines the effect of applying two or more deltas into one.  Since I haven't finished testing this feature, the code is only available in subversion. See <a href="http://code.google.com/p/xdelta/source/checkout">here</a> and enjoy.
</div>
