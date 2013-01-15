default: bin/aobench-debug.apk

bin/aobench-debug.apk: local.properties jni/main.c jni/aobench.c jni/aobench.h jni/font.c jni/font.h jni/config.h
	@ndk-build NDK_DEBUG=0
	@ant debug
	@mv bin/aobench-debug.apk bin/aobench.apk
	@-adb uninstall jp.aobench.android.native_activity
	@adb install bin/aobench.apk

local.properties:
	@android update project -p . -n aobench

clean:
	rm -rf bin obj libs

fullclean: clean
	rm -f build.xml local.properties proguard-project.txt

