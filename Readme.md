Colibri Gui library
===================
 * https://bitbucket.org/dark_sylinc/colibrigui/ (mercurial)
 * https://github.com/darksylinc/colibrigui/ (git)

We use [hg-git plugin](https://github.com/schacon/hg-git) to
interoperate between Mercurial & Git.

It's simple: I prefer Mercurial over Git
[a lot](http://www.yosoygames.com.ar/wp/2016/08/git-the-linux-kernel-and-other-projects/),
but I recognize Github's tools are superior (community, ticket system, pull requests)-

Any Pull Request or Ticket must be submitted to Github repo and must not break hg-git
interoperability (you have to do very weird git branch stuff to break it).

Originally this project was called "Crystal Gui", however googling around revealed this
name was already taken by something completely unrelated. To prevent clashing, it
was thus renamed to Colibri Gui, which doesn't seem to clash with anything at the
time of the rebrand.


Goals
=====

1. Be lightweight within reasonable expectations. Except for text rendering.
1. Text rendering dependencies: Be a library that has no or minimal external dependencies.
We perform text rendering using Freetype2 & HarfBuzz. Additionally these two depend on
ZLib. These libraries are being included and have been tuned with defaults to use our own
compiled versions, rather than system ones. The libraries are not using CoreText, Direct2D
nor any other OS-specific rendering implementation. This allows Colibri's text engine
to be used on virtually any platform (e.g. Linux, Windows, Mac, iOS, Android) in a
consistent manner, and without having to worry about ABI breakage.
1. Freetype2 depends on HarfBuzz (optional) to enhance auto hinting. But for that to work,
HarfBuzz must be compiled with Freetype2. That's a chicken and egg problem.
This can only solved if both libraries are static, or compiled as a single dll, or
compiled multiple times iteratively until both dependencies are met. We chose to go
for static libs.


Missing text features and Limitations
=====================================

1. Automatically substitute fonts when a string has mixed characters, and the selected
font cannot represent that character, while another font can. In other words, no font
detection: If you have multiple fonts e.g. Latin and Japanese, and you use
a japanese character while using the latin font, we won't automatically use the japanese
font and end up displaying a box instead.
1. CJK Top to Bottom: Draw 2-digit numbers at the same height instead of two.
1. LinebreakMode::WordWrap assumes space and tabs is what separate words, which is not
always true for all languages.

Known issues
============

1. Mixing multiple font sizes into the same Label, the correct height for the newline
will not always be correctly calculated and thus be overestimated.


How to build docs
=================

You'll need:

1. CMake 3.9 or higher
1. Doxygen. `sudo apt install doxygen`

Create the CMake script and type: `ninja doxygen`


Unicode mini-intro for contributors
===================================

Unicode is hard. But it's harder than it should be because there's a lot of misconceptions
and misinformation. To make it even harder, the technical reports from unicode.org are
far from user friendly.

This mini-intro is aimed at preventing you from making the common mistakes.

Asuming we're dealing with UTF-8, Unicode has several things you need to take in mind:

1. Code unit: A unicode codeunit in UTF-8 correspond with a byte.
1. Code unit: A codeunit is formed from one or more codeunits. For example the letter
'A' is represented by the codeunit '0x41' in UTF-8, while the kanji codeunit '漢' is
represented by the three-byte sequence 'E6 BC A2'.<br/>
It is a common misconception to think that a code unit is a character. In UTF-32,
codeunits are the same as codeunits.
1. 'Glyph' / 'Grapheme cluster': A grapheme cluster is made from one or multiple code
units.
Grapheme clusters can be caused by multiple reasons:
	1. Precomposed characters vs decomposed: For example the character ö can be
  	written in two ways: its precomposed form ö or its decomposed form o + ¨
		* Precomposed ö codeunit U+00F6; codeunits C3 B6
		* Decomposed ö codeunits U+006F and U+0308; codeunits 6F CC 88<br/>
	Note that there can be more than two codeunits. For example the glyph ṓ is made up
	from 3 codeunits, but should be rendered as a single character
	1. Complex text layout: For example in arabic scripts, the same letter can be
	represented in three different ways, depending on whether the letter is at the
	start, in the middle or at the beginning of a word. This is only one such example.
	We rely on HarfBuzz library to perform Complex Text Layout for us. 

So what really counts as a "character" or "letter" is more accurately a grapheme cluster
or glyph, not a code unit. We store the beginning of the cluster (i.e. the first
codeunit) in a string in ShapedGlyph::clusterStart

*More information:*

 * [Let’s Stop Ascribing Meaning to Code Points](https://manishearth.github.io/blog/2017/01/14/stop-ascribing-meaning-to-unicode-code-points/)
 * [Breaking Our Latin-1 Assumptions](https://manishearth.github.io/blog/2017/01/15/breaking-our-latin-1-assumptions/)
 * [Arabic Localization of a Game – Our Experience](https://forum.unity.com/threads/arabic-localization-our-experience.355100/)
 * [Precomposed Character - Wikipedia](https://en.wikipedia.org/wiki/Precomposed_character)
 * [Complex text layout - Wikipedia](https://en.wikipedia.org/wiki/Complex_text_layout)
 * [Using a Japanese IME](http://blog.gatunka.com/2009/09/12/using-a-japanese-ime/)
 * [IME Basics for Developers](http://blog.gatunka.com/2009/09/20/ime-basics-for-developers/)
