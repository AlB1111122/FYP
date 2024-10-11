sdl:
	g++ -o build/new_display src/display_sdl.cpp src/filter.cpp `pkg-config --cflags --libs sdl2 glew`