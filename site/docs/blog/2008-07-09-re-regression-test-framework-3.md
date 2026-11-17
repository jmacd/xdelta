---
original_title: ""
date: "2008-07-09T21:00:00.000-07:00"
updated: "2008-07-09T21:51:44.777-07:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2008/07/re-regression-test-framework-3.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-1289505834152466908"
---

# Post from 2008-07-09 21:00

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2008-07-09T21:00:00.000-07:00 | [Original post](https://xdelta.blogspot.com/2008/07/re-regression-test-framework-3.html)

<div class="blogger-post">
Re: Regression test framework<br /><br />3.0t released in December 2007 has turned out to be extremely stable, and I'm busy preparing the first non-beta release candidate.  The problem with being stable is the risk of regression, so I've been nervously putting together a new regression testing framework to help exercise obscure bugs.<br /><br />The most critical bug reported since 3.0t is a non-blocking API-specific problem.  Certain bugs only affect the API, not the <tt>xdelta3</tt> command-line application, because the command-line application uses blocking I/O even though the API supports non-blocking I/O.  <a href="http://code.google.com/p/xdelta/issues/detail?id=70">Issue 70</a> reported an infinite loop processing a certain pair of files.  The reporter graciously included sample inputs to reproduce the problem, and I fixed it in SVN 239.  <br /><br />But I wasn't happy with the fix until now, thanks to the new regression testing framework.  With ~50 lines of code, the test creates an in-memory file descriptor, then  creates two slight variations intended to trigger the bug.  SVN 256 contains the new test.<br /><br />So just what is it that makes me feel 3.x is so stable?  It's e-mails like this from Alex White at intralan.co.uk:<ul>Hi Josh,<br /><br />FYI:-<br /><br />Been using XDelta for a while now, been working flawlessly (I wish all software could be this stable), been patching up to 1tb of data per day (across many servers), largest single file to date 70gb!!!<br /><br />Did some performance testing and with standard SATAII drives with both sources and the patch being created on the same drive the processing was around 300mb per minute (creating patches), setup a dual drive configuration where one of the sources was on a second drive and then the processing was around 1gb per minute (creating patches) .<br /><br />Best large file patch to original file size ratio,<br /><br />Original file size:  56,147,853,312<br /><br />Patch file size: 299,687,049<br /><br />Patch file reduction over original file size 99.47%<br /><br />This file took 2 hours and 21 minutes to patch in a real world setting.</ul>Thanks for all the feedback! I like stable software too.
</div>
