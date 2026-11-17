---
original_title: ""
date: "2006-07-02T18:34:00.000-07:00"
updated: "2006-07-02T18:44:13.546-07:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2006/07/thanks-for-your-continuing-reports.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-115189105353617566"
---

# Post from 2006-07-02 18:34

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2006-07-02T18:34:00.000-07:00 | [Original post](https://xdelta.blogspot.com/2006/07/thanks-for-your-continuing-reports.html)

<div class="blogger-post">
Thanks for your continuing reports. Release <a href="http://prdownloads.sourceforge.net/xdelta/xdelta30f.tar.gz">3.0f</a> fixes a bug in <tt>xd3_iopt_flush_instructions</tt>:<br /><pre><br />   /* If forcing, pick instructions until the list is empty, otherwise this empties 50% of<br />    * the queue. */<br />   for (flushed = 0; ! xd3_rlist_empty (& stream->iopt.used); )<br />     {<br />-      if ((ret = xd3_iopt_add_encoding (stream,<br />-                    xd3_rlist_pop_front (& stream->iopt.used)))) { return ret; }<br />-      // TODO: what about this fraction??<br />-      if (! force && ++flushed > stream->iopt_size / 2) { break; }<br />+      xd3_rinst *renc = xd3_rlist_pop_front (& stream->iopt.used);<br />+      if ((ret = xd3_iopt_add_encoding (stream, renc)))<br />+       {<br />+         return ret;<br />+       }<br />+<br />+      if (! force) <br />+       {<br />+         if (++flushed > stream->iopt_size / 2)<br />+           {<br />+             break;<br />+           }<br />+<br />+         /* If there are only two instructions remaining, break, because they were<br />+          * not optimized.  This means there were more than 50% eliminated by the<br />+          * loop above. */<br />+         r1 = xd3_rlist_front (& stream->iopt.used);<br />+         if (xd3_rlist_end(& stream->iopt.used, r1) ||<br />+             xd3_rlist_end(& stream->iopt.used, r2 = xd3_rlist_next (r1)) ||<br />+             xd3_rlist_end(& stream->iopt.used, r3 = xd3_rlist_next (r2)))<br />+           {<br />+             break;<br />+           }<br />+       }<br />     }<br /></pre>
</div>
