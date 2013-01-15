#ifndef AOBENCH_H
#define AOBENCH_H

void aobench_init_scene();
void aobench_render(unsigned char *img, int w, int h, int nsubsamples, int y);
void aobench_saveppm(const char *fname, int w, int h, unsigned char *img);

#endif // AOBENCH_H
