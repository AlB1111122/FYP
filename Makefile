SRC_DIR = src/platform/baremetal
OBJ_DIR = build/bm
CFG_DIR = src/platform/baremetal/config

CFILES = $(wildcard $(SRC_DIR)/*.c)
OFILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CFILES))

GCCFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -mstrict-align
GCCPATH = /usr/share/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf/bin

all: clean kernel8.img

$(OBJ_DIR)/boot.o: $(CFG_DIR)/boot.S
	mkdir -p $(OBJ_DIR)
	$(GCCPATH)/aarch64-none-elf-gcc $(GCCFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(GCCPATH)/aarch64-none-elf-gcc $(GCCFLAGS) -c $< -o $@

kernel8.img: $(OBJ_DIR)/boot.o $(OFILES)
	$(GCCPATH)/aarch64-none-elf-ld -nostdlib $(OBJ_DIR)/boot.o $(OFILES) -T $(CFG_DIR)/link.ld -o $(OBJ_DIR)/kernel8.elf
	$(GCCPATH)/aarch64-none-elf-objcopy -O binary $(OBJ_DIR)/kernel8.elf $(OBJ_DIR)/kernel8.img

PHONY: clean
clean:
	/bin/rm -f $(OBJ_DIR)/* > /dev/null 2> /dev/null || true