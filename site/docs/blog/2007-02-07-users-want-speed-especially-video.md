---
original_title: ""
date: "2007-02-07T19:00:00.000-08:00"
updated: "2007-02-08T01:39:11.113-08:00"
authors: ["jmacd"]
original_url: "https://xdelta.blogspot.com/2007/02/users-want-speed-especially-video.html"
blogger_id: "tag:blogger.com,1999:blog-6565998.post-117090989064567734"
---

# Post from 2007-02-07 19:00

!!! note "Historical post"
    This is an archived post. Links, licensing statements, and technical guidance
    may be out of date. See the [current documentation](../index.md).

Published 2007-02-07T19:00:00.000-08:00 | [Original post](https://xdelta.blogspot.com/2007/02/users-want-speed-especially-video.html)

<div class="blogger-post">
Users want speed, especially video gamers. I tested with some of your data: <a href="http://openut.sourceforge.net/">Unreal tournament</a> and <a href="http://www.wesnoth.org/">Wesnoth</a> patches. These patches save 50-100MB per download.<br /><br />Xdelta1 remains popular today because of speed, and xdelta3 until now hasn't been as fast (<a href="http://tonelli.sns.it/pub/mennucc1/debdelta/tests/">debdev</a> has tests). Xdelta-3.0o has improved compression levels (<a href="http://code.google.com/p/xdelta/downloads/list">download</a>).<br /><br />Over my sample data, the new default compression level (same as the command-line flag <tt>-1</tt>) is a little faster than, with compression roughly the same as Xdelta-1.x.  Compression levels <tt>-3</tt>, <tt>-6</tt>, and <tt>-9</tt> are also improved.<br /><br />This release also features <a href="http://code.google.com/p/xdelta/wiki/LanguageInterface">Swig support</a>, e.g.,<ul><tt><pre>import xdelta3<br /><br /># memory interface<br />source = 'source source input0 source source'<br />target = 'source source target source source'<br /><br />result1, patch = xdelta3.xd3_encode_memory(target, source, len(target))<br />result2, target1 = xdelta3.xd3_decode_memory(patch, source, len(target))<br /><br />assert result1 == 0<br />assert result2 == 0<br />assert target1 == target<br />assert len(patch) < len(target)<br /><br /># command-line interface<br /><br />source_file = ...<br />target_file = ...<br />command_args = ['xdelta3', '-f', '-s', source_file, target_file, output_file]<br /><br />xdelta3.xd3_main_cmdline(command_args)</pre></tt></ul>
</div>
