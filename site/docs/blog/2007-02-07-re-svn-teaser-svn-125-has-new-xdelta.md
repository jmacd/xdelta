---
original_title: ""
date: "2007-02-07T08:00:00.000-08:00"
updated: "2007-02-08T08:36:04.986-08:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2007/02/re-svn-teaser-svn-125-has-new-xdelta.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-117094704154990225"
---

# Post from 2007-02-07 08:00

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2007-02-07T08:00:00.000-08:00 | [Original post](https://xdelta.blogspot.com/2007/02/re-svn-teaser-svn-125-has-new-xdelta.html)

<div class="blogger-post">
Re: SVN teaser<br /><br />SVN 125 has a new <tt>XDELTA</tt> environment variable for passing flags off the command-line, so you can use xdelta3 in conjunction with tar:<ul><tt><pre>export XDELTA<br />XDELTA="-s source-1.tar"<br />TAR="tar --use-compress-program=xdelta3"<br />$TAR -cvf source-1-source2.tar.vcdiff source-2/<br />...<br />$TAR -xvf source-1-source2.tar.vcdiff source-2/file</pre></tt></ul>This creates/extracts-from a delta-compressed tar file, without using intermediate files.
</div>
