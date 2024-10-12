build-linuxv1:
	g++ -o build/linuxv1 src/display_sdl.cpp src/filter.cpp `pkg-config --cflags --libs sdl2 glew`

linuxv1:
	build/linuxv1 intersection.mpg
