#bm
CROSSCOMP_TOOLCHAIN_DIR = /usr/share/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf
ETL_INSTALL_DIR = /usr/share/etl/etl-20.39.4

SRC_DIR = src/platform/baremetal
SRC_COMM_DIR = src/common
OBJ_DIR = build/bm
CFG_DIR = src/platform/baremetal/config

#if changing optimization level only change OPTIMIZE_LEVEL. OP_FLAG is used so the code can tell if O3 is enabled the if handles it
OP_FLAG =
OPTIMIZE_LEVEL = 3
ifeq ($(OPTIMIZE_LEVEL), 3)
    OP_FLAG := -DOP_FLAG
endif

CFILES = $(wildcard $(SRC_DIR)/*.cc) $(wildcard $(SRC_COMM_DIR)/*.cc)
OFILES = $(patsubst $(SRC_DIR)/%.cc, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.cc)) \
         $(patsubst $(SRC_COMM_DIR)/%.cc, $(OBJ_DIR)/%.o, $(wildcard $(SRC_COMM_DIR)/*.cc))

LINKLIBS = -L$(CROSSCOMP_TOOLCHAIN_DIR)/aarch64-none-elf/lib
LINKFLAG = -lc -lg -lm
#will not work without them
MANDATORY_COMP_FLAGS =  -ffreestanding -nostdinc -nostdlib -nostartfiles -mstrict-align -fno-exceptions -fpermissive
COMPFLAGS = $(MANDATORY_COMP_FLAGS) -Wall -O$(OPTIMIZE_LEVEL) $(OP_FLAG) -ftree-vectorize -march=armv8-a+simd -mcpu=cortex-a72 -mtune=cortex-a72 -falign-functions=32 -falign-loops=32
INCLFLAGS = -I$(CROSSCOMP_TOOLCHAIN_DIR)/aarch64-none-elf/include -I$(ETL_INSTALL_DIR)/include -I$(CROSSCOMP_TOOLCHAIN_DIR)/lib/gcc/aarch64-none-elf/13.3.1/include -I./include -I./include/common -I./lib
GCCFLAGS = $(LINKLIBS) $(COMPFLAGS) $(INCLFLAGS)
GCCPATH = $(CROSSCOMP_TOOLCHAIN_DIR)/bin

all: clean kernel8.img

$(OBJ_DIR)/boot.o: $(CFG_DIR)/boot.S
	mkdir -p $(OBJ_DIR)
	$(GCCPATH)/aarch64-none-elf-g++ $(GCCFLAGS) -c $< -o $@ $(LINKFLAG)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc
	mkdir -p $(OBJ_DIR)
	$(GCCPATH)/aarch64-none-elf-g++ $(GCCFLAGS) -c $< -o $@ $(LINKFLAG)

$(OBJ_DIR)/%.o: $(SRC_COMM_DIR)/%.cc
	mkdir -p $(OBJ_DIR)
	$(GCCPATH)/aarch64-none-elf-g++ $(GCCFLAGS) -c $< -o $@ $(LINKFLAG)

kernel8.img: $(OBJ_DIR)/boot.o $(OFILES)
	$(GCCPATH)/aarch64-none-elf-ld $(LINKLIBS) -nostdlib $(OBJ_DIR)/boot.o $(OFILES) -T $(CFG_DIR)/link.ld -o $(OBJ_DIR)/kernel8.elf $(LINKFLAG) -Map=build/bm/output.map
	$(GCCPATH)/aarch64-none-elf-objcopy -O binary $(OBJ_DIR)/kernel8.elf $(OBJ_DIR)/img/kernel8.img

.PHONY: clean build
clean:
	/bin/rm -f $(OBJ_DIR)/* $(OBJ_DIR)/img/* > /dev/null 2> /dev/null || true

build:
	mkdir -p $(OBJ_DIR)/img

#linux
deep-clean-l:
	rm -rf build && mkdir -p $(OBJ_DIR)/img;

scratch-build-l:
	cd build;cmake ..;make;

run:
	build/$(PROJ_NAME) $(VIDEO_NAME).mpg;

.PHONY: rebuild
rebuild:
	cd build;make;

clean-l:
	cd build;make clean;
