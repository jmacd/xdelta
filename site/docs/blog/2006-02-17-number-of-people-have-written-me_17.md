---
original_title: ""
date: "2006-02-17T13:00:00.000-08:00"
updated: "2006-02-18T17:09:28.070-08:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2006/02/number-of-people-have-written-me_17.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-114021095821293447"
---

# Post from 2006-02-17 13:00

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2006-02-17T13:00:00.000-08:00 | [Original post](https://xdelta.blogspot.com/2006/02/number-of-people-have-written-me_17.html)

<div class="blogger-post">
A number of people have written me recently regarding xdelta3 on windows. Fact is, I would love it. One writes:<pre><br />> Hello,<br />><br />> I have compiled xdelta with cygwin. I have noticed several versions of<br />> executables produced, but my question is quite simple.<br />> I want to make diff-like delta from one file to another in the way I could<br />> in xdelta 1.x, the syntax was<br />><br />>     xdelta delta original_file new_file delta_file<br />><br />> and to create new file from delta<br />><br />>     xdelta patch delta_file original_file new_file<br />><br />> What combination of arguments and what commands shall I issue with xdelta3<br />> to gain the same effect?<br /></pre><br />The syntax is similar to gzip, with the addition of a "-s original_file" argument. By themselves,<br /><br />xdelta3 -df<br />xdelta3 -f<br /><br />operate the similarly to gzip, even without the -s flag.<br /><br /><pre><br />> I can't figured out what is wrong with the module.  But I solve the<br />> problem calling the C binary from os.system('/usr/bin/xdelta3...').<br />> <br />> I whish to tell you that I'm very impressed by xdelta3 and would like<br />> to see finished and polished.  I just hope you have some time to<br />> deliver an xdelta3 release.<br />> <br />> Again, thank you for xdelta3!<br /></pre><br /><br />You're welcome. Thanks for writing in. I have a big pile of mail still to go through, and I have at least one xdelta3 bug report. Stay tuned...
</div>
