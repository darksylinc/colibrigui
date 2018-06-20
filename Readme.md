Crystal Gui library
===================
 * https://bitbucket.org/dark_sylinc/crystalgui/ (mercurial)
 * https://github.com/darksylinc/crystalgui/ (git)

We use [https://github.com/schacon/hg-git](hg-git plugin) to
interoperate between Mercurial & Git.

It's simple: I prefer Mercurial over Git
[http://www.yosoygames.com.ar/wp/2016/08/git-the-linux-kernel-and-other-projects/](a lot),
but I recognize Github's tools are superior (community, ticket system, pull requests)-

Any Pull Request or Ticket must be submitted to Github repo and must not break hg-git
interoperability (you have to do very weird git branch stuff to break it).


Goals
=====

1. Be lightweight within reasonable expectations. Except for text rendering.
1. Text rendering dependencies: Be a library that has no or minimal external dependencies.
We perform text rendering using Freetype2 & HarfBuzz. Additionally these two depend on
ZLib. These libraries are being included and have been tuned with defaults to use our own
compiled versions, rather than system ones. The libraries are not using CoreText, Direct2D
nor any other OS-specific rendering implementation. This allows CrystalGui's text engine
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
