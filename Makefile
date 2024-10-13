deep-clean:
	rm -rf build && mkdir build;

scratch-build:
	cd build;cmake ..;make;

run:
	build/$(PROJ_NAME) $(VIDEO_NAME).mpg;

.PHONY: rebuild
rebuild:
	cd build;make;

clean:
	cd build;make clean;
