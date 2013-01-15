#include <jni.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "aobench", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "aobench", __VA_ARGS__))

#include "config.h"
#include "aobench.h"
#include "font.h"


struct engine
{
	struct android_app *app;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int rendering;
	int width, height;
	int frame, row;
	int curmsec, lastmsec;
	unsigned char *img;
	float texu, texv;
	GLuint texid;
};

// round value to next power of two (or return same if already a power of two)
static int nextPow2(int val)
{
	int next = 1;
	
	while (next < val)
		next <<= 1;
	
	return next;
}

// get current time in milliseconds
static int getMsec()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}

// initialize an EGL context for the current display
static int engine_init_display(struct engine *engine)
{
	// initialize OpenGL ES and EGL
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
		EGL_BLUE_SIZE,		8,
		EGL_GREEN_SIZE,		8,
		EGL_RED_SIZE,		8,
		EGL_NONE
	};

	EGLint w, h, dummy, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(display, 0, 0);

	// pick the first EGLConfig that matches our criteria
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
	ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
	{
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	LOGI("display: width=%d, height=%d", w, h);

#ifdef AOBENCH_FULLSCREEN
	engine->width  = w;
	engine->height = h;
#else
	engine->width  = 256;
	engine->height = 256;
#endif

	LOGI("render: width=%d, height=%d", engine->width, engine->height);
	
	// calculate appropriate texture size and UV coordinates
	int texwidth  = nextPow2(engine->width);
	int texheight = nextPow2(engine->height);
	engine->texu = engine->width  / (float)texwidth;
	engine->texv = engine->height / (float)texheight;
	
	LOGI("texture: width=%d, height=%d", texwidth, texheight);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;

	// initialize GL state
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	
	// set orthographic projection and view matrices to use fullscreen pixel coordinates
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0, w, h, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// generate and initialize a texture to display the rendered aobench scene
	glGenTextures(1, &engine->texid);
	glBindTexture(GL_TEXTURE_2D, engine->texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texwidth, texheight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	
	// initialize aobench scene
	aobench_init_scene();
	
	// allocate memory for the aobench render buffer
	const int size = engine->width * engine->height * 3;
	engine->img = (unsigned char *)malloc(size);
	memset(engine->img, 0, size);
	
	// initialize engine rendering state
	engine->row		  = 0;
	engine->frame	  = 0;
	engine->curmsec	  = 0;
	engine->lastmsec  = 0;
	engine->rendering = 1;
	return 0;
}

// save framebuffer image to file
static void engine_save_buffer(struct engine *engine)
{
	JavaVM *vm;
	JNIEnv *env;
	jclass clazz;
	jmethodID method;
	jobject obj;
	jstring str;
	char fname[256];
	
	// first attach our thread to the VM so we can get a JNI environment pointer
	vm = engine->app->activity->vm;
	(*vm)->AttachCurrentThread(vm, &env, NULL);
	
	// lookup the external storage directory
	clazz = (*env)->FindClass(env, "android/os/Environment");
	method = (*env)->GetStaticMethodID(env, clazz, "getExternalStorageDirectory", "()Ljava/io/File;");
	obj = (*env)->CallStaticObjectMethod(env, clazz, method);

	// convert it to a string
	clazz = (*env)->GetObjectClass(env, obj);
	method = (*env)->GetMethodID(env, clazz, "getAbsolutePath", "()Ljava/lang/String;");
	str = (jstring)(*env)->CallObjectMethod(env, obj, method);

	// get a pointer to the string and create the output .ppm filename
	const char *path = (*env)->GetStringUTFChars(env, str, NULL);
	snprintf(fname, sizeof(fname), "%s/aobench-android.ppm", path);
	(*env)->ReleaseStringUTFChars(env, str, path);
	
	// save the .ppm
	aobench_saveppm(fname, engine->width, engine->height, engine->img);
}

// render the current frame in the display
static void engine_draw_frame(struct engine *engine)
{
	const GLfloat verts[] = {
		engine->width,	0.0f,
		0.0f,			0.0f,
		engine->width,	engine->height,
		0.0f,			engine->height
	};
	
	const GLfloat texcoords[] = {
		engine->texu,	0.0f,
		0.0f,			0.0f,
		engine->texu,	engine->texv,
		0.0f,			engine->texv
	};
	
	if (engine->display == NULL)
		return;

	// clear the display to black
	glClearColor(0.0f, 0.0f, 0.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// time and render aobench for the current row
	const int begin = getMsec();
	aobench_render(engine->img, engine->width, engine->height, AOBENCH_NSUBSAMPLES, engine->row++);
	engine->curmsec += getMsec() - begin;
	
	// check if we rendered a full frame
	if (engine->row >= engine->height)
	{
		// if this was the first frame, then save the aobench buffer to SD flash
		if (engine->frame == 0)
			engine_save_buffer(engine);

		// reset rendering for the next frame
		engine->lastmsec = engine->curmsec;
		engine->curmsec	 = 0;
		engine->row		 = 0;
		engine->frame++;
	}

	// update the GL texture
	glBindTexture(GL_TEXTURE_2D, engine->texid);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, engine->width, engine->height, GL_RGB, GL_UNSIGNED_BYTE, engine->img);

	// setup rendering
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// render the polygon
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	// print timing info
	const int msec = (engine->lastmsec != 0) ? engine->lastmsec : engine->curmsec;
	font_printf(0, 0, 2.0f, "aobench (%dx%d, %s): %d ms / %.3f fps, frame #%d, row #%d", engine->width, engine->height, 
#ifdef AOBENCH_FLOAT
				"float",
#else
				"double",
#endif
				msec, 1.0f / (msec / 1000.0f), engine->frame, engine->row);
	
	// and show the rendered surface to the display
	eglSwapBuffers(engine->display, engine->surface);
}

// tear down the EGL context currently associated with the display
static void engine_term_display(struct engine *engine)
{
	if (engine->display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (engine->context != EGL_NO_CONTEXT)
			eglDestroyContext(engine->display, engine->context);

		if (engine->surface != EGL_NO_SURFACE)
			eglDestroySurface(engine->display, engine->surface);

		eglTerminate(engine->display);
	}

	engine->rendering = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

// process the next main command
static void engine_handle_cmd(struct android_app *app, int32_t cmd)
{
    struct engine *engine = (struct engine *)app->userData;

	switch (cmd)
	{
		case APP_CMD_INIT_WINDOW:
			// the window is being shown, so initialize it
			if (engine->app->window != NULL)
			{
				engine_init_display(engine);
				engine_draw_frame(engine);
			}
			break;

		case APP_CMD_TERM_WINDOW:
			// the window is being hidden or closed, clean it up
			engine_term_display(engine);
			break;

		case APP_CMD_GAINED_FOCUS:
			// our app gained focus, so restart rendering
			engine->rendering = 1;
			break;

		case APP_CMD_LOST_FOCUS:
			// our app lost focus, stop rendering
			engine->rendering = 0;
			engine_draw_frame(engine);
			break;
	}
}

// the main entry point of a native application that uses android_native_app_glue.  it runs in its own thread, with its own event loop for receiving input events.
void android_main(struct android_app *state)
{
	struct engine engine;

	// make sure glue isn't stripped
	app_dummy();

	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	engine.app = state;

	// loop waiting for stuff to do
	while (1)
	{
		// read all pending events
		int ident;
		int events;
		struct android_poll_source *source;

		// if not rendering, we will block forever waiting for events.
		// if we are rendering, we loop until all events are read, then continue to draw the next frame of animation.
		while ((ident = ALooper_pollAll(engine.rendering ? 0 : -1, NULL, &events, (void **)&source)) >= 0)
		{
			// process this event
			if (source != NULL)
				source->process(state, source);

			// check if we are exiting
			if (state->destroyRequested != 0)
			{
				engine_term_display(&engine);
				return;
			}
		}

		// done with events, so render next frame
		if (engine.rendering)
			engine_draw_frame(&engine);
	}
}
