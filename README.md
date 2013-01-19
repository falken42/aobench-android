This is a port of aobench ( http://code.google.com/p/aobench/ ) for Android 2.3.3 (API 10) or higher ARM devices with a NEON FPU, including the OUYA and Kindle Fire HD.  aobench is a small ambient occlusion renderer for benchmarking real world floating point performance in various languages.  Please see the project page for more details.

Benchmarks:
-----------
OUYA (256x256, float): 5,401ms (5.4sec)

![Rendering](http://falken42.github.com/aobench-ouya-256x256-float.jpg)

https://twitter.com/_Falken/status/292602217083576320

OUYA (1920x1080, float): 169,805ms (169.8sec / 2.83min)

![Rendering](http://falken42.github.com/aobench-ouya-1920x1080-float.jpg)

https://twitter.com/_Falken/status/292602468922163200

Kindle Fire HD (256x256, float): 7,732ms (7.7sec)

![Rendering](http://falken42.github.com/aobench-kindlefirehd-256x256-float.jpg)

https://twitter.com/_Falken/status/292602683444056064

Kindle Fire HD (256x256, double): TBD

Kindle Fire HD (1280x800, float): 120,946ms (120.9sec / 2.01min)

![Rendering](http://falken42.github.com/aobench-kindlefirehd-1280x800-float.jpg)

https://twitter.com/_Falken/status/292602896128823296

Kindle Fire HD (1280x800, double): TBD

How to build:
-------------
1) Ensure the Android SDK and NDK are installed with API 10 support.

2) Type 'make' in the source folder.  All necessary files will be generated.

3) Tweak config.h if you want to change float/double or 256x256/fullscreen mode.

Thanks/greets:
--------------
Thanks to @syoyo for creating the interesting aobench project!

