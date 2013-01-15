#ifndef CONFIG_H
#define CONFIG_H

// if #defined, aobench uses float instead of double (default: double)
//#define AOBENCH_FLOAT

// if #defined, aobench renders the full size of the display, otherwise renders 256x256 (default: 256x256)
//#define AOBENCH_FULLSCREEN

// number of subsamples (default: 2)
#define AOBENCH_NSUBSAMPLES		2

// number of ambient occlusion samples (default: 8)
#define AOBENCH_NAO_SAMPLES		8

#endif // CONFIG_H
