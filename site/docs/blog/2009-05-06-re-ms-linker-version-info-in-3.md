---
original_title: ""
date: "2009-05-06T18:19:00.000-07:00"
updated: "2009-05-06T18:37:35.462-07:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2009/05/re-ms-linker-version-info-in-3.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-7413656429112129773"
---

# Post from 2009-05-06 18:19

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2009-05-06T18:19:00.000-07:00 | [Original post](https://xdelta.blogspot.com/2009/05/re-ms-linker-version-info-in-3.html)

<div class="blogger-post">
Re: MS "Linker Version Info" in 3.0u<br /><br />I can't explain this problem report *SEE BELOW*, indicating that Xdelta on Microsoft platforms is slower than the previous build by a factor of two.<br /><br />?<br /><pre><br />Hello ... !<br /><br />     First of all I want to thank you for creating XDelta tool.<br />Realy nice and well documented stuff which sometimes helps a lot.<br />     When I changed from v3.0t to v3.0u I've noticed some slowdown<br />in processing time. Well, it was just visual observation so I decided<br />to conduct a couple of tests. Here's what I've done.<br />     Test was conducted on virtual memory disk using RAMDiskXP v2.0.0.100<br />from Cenatek Inc. It was done to exclude any impact from IO system.<br />Processing time was measured with Timer v8.00 tool by Igor Pavlov.<br />Test files size is about 275 MB. More exactly speaking 288 769 595<br />bytes for Test.dat.old and 288 771 262 bytes for Test.dat<br />Command line for XDelta is:<br />timer xdelta3 -v -9 -A= -B x -W x -I 0 -P x -s Test.dat.old Test.dat Diff.dat<br />where x is one of the values given below with the test results.<br /><br />               3.0t    3.0u<br />               -----   -----   --------------<br />16384           3.687   6.391   73.3% slower<br />65536           3.469   6.125   76.6% slower<br />1048576         2.578   5.453   111.5% slower<br />4194304         3.281   6.625   101.9% slower<br />8388608         3.953   7.718   95.2% slower<br /><br />As you see v3.0u is averagely 91.7% SLOWER !!! I don't think it's a<br />some evil coincidence cause I redone every test twice. I have only one<br />clue. I see Linker Version Info in v3.0t exe-file is 8.0 while is 9.0<br />for the v3.0u exe-file so I suppose you changed or compilator itself<br />or its version.<br />  I'll be very glad to hear from you.<br />  Truly yours, ...<br /></pre>
</div>
