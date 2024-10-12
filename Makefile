clean:
	rm -rf build && mkdir build;

build: clean
	cd build;cmake ..;make;

run:
	build/$(PROJ_NAME) $(VIDEO_NAME).mpg;
