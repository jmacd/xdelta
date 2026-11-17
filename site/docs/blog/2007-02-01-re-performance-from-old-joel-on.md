---
original_title: ""
date: "2007-02-01T22:00:00.000-08:00"
updated: "2007-02-02T00:08:22.973-08:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2007/02/re-performance-from-old-joel-on.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-117040060331037183"
---

# Post from 2007-02-01 22:00

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2007-02-01T22:00:00.000-08:00 | [Original post](https://xdelta.blogspot.com/2007/02/re-performance-from-old-joel-on.html)

<div class="blogger-post">
Re: Performance<br /><br />From <a href="http://discuss.fogcreek.com/joelonsoftware/default.asp?cmd=show&ixPost=2931">The Old Joel on Software Forum</a>:<ul>I checked out rtpatch and xdelta a little while ago - unless there's something wrong with the demo version rtpatch has on their website, xdelta is significantly faster at making patches, and they are a tiny bit smaller.<br /><br />The test I ran was on a 200 Meg application, where about 30 megs of random parts of files was different. Rtpatch took around an hour (not sure since I wandered off while waiting) and xdelta took a couple minutes.<br /><br />We ended up rolling our own system anyhow, because our application has pretty serious requirements on robustness. Maybe we should try selling it?</ul>Thanks! 5 years later.<br /><br />Another post in the same thread writes:<ul>I did some research on 'patching' like issues, read some papers on the "Longest Common Subsequence" problem, wrote some prototype code...  And we ultimately decided to defer the issue in our product.  Note that we weren't trying to patch our software, we were interested in including tools in our product that would allow administrators to build patches for applications they support in house.<br /><br />Long story short...it's a heavily researched issue in Computer Science, but it's still a difficult problem to solve well.  There are both space and time considerations, although running on a 'modern' OS with virtual memory eases the space consideration.  After my evaluation, my guess was that RTPatch was in no way producing an optimal solution, but that they were relying on compression to keep the size of the patch small.  Their genius lies in being able to generate a patch so quickly...</ul><a href="http://en.wikipedia.org/wiki/Longest_common_subsequence">Longest common subsequence</a> isn't quite the same problem, but it's true that compression performance should be measured in several dimensions: size (of compression), memory usage, and speed.   Xdelta3 supports compression levels -1 (fast) through -9 (best).<br /><br />Virtual memory does not ease the space consideration. Reading from disk is terribly slow, so Xdelta3 avoids seeking backwards in the source file during compression.  Read more about xdelta3 <a href="http://code.google.com/p/xdelta/wiki/TuningMemoryBudget">memory</a> settings.
</div>
