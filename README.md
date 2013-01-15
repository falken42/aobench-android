This is a port of aobench ( http://code.google.com/p/aobench/ ) for Android 2.3.3 (API 10) or higher devices, including the OUYA and Kindle Fire HD.  aobench is a small ambient occlusion renderer for benchmarking real world floating point performance in various languages.  Please see the project page for more details.

Benchmarks:
-----------
OUYA (256x256, float): 33260ms

OUYA (1920x1080, float): 1048888ms

Kindle Fire HD (256x256, float): 46625ms

Kindle Fire HD (256x256, double): 65174ms

Kindle Fire HD (1280x800, float): 730170ms

Kindle Fire HD (1280x800, double): TBD

How to build:
-------------
1) Ensure the Android SDK and NDK are installed with API 10 support.

2) Type 'make' in the source folder.  All necessary files will be generated.

3) Tweak config.h if you want to change float/double or 256x256/fullscreen mode.

Thanks/greets:
--------------
Thanks to @syoyo for creating the interesting aobench project!

