
# Goals

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
