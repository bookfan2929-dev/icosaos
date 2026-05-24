# Cross-compiler
CROSS = ~/cross/bin
CC = $(CROSS)/i686-elf-gcc
LD = $(CROSS)/i686-elf-ld
OBJCOPY = $(CROSS)/i686-elf-objcopy

# CFLAGS: Changed to use GCC for assembly files so they can share headers with C code.
# -g keeps debug symbols intact if you need to trace symbols across files later.
CFLAGS = -ffreestanding -O0 -Wall -Wextra -m32 -g
LDFLAGS = -T linker.ld -nostdlib -m elf_i386

SRC = src
KERNSRC = $(SRC)/kernel
BUILD = build
IMG = disk.img
FATIMG = fat.img

ENTRY = $(BUILD)/entry.o
KERNEL_OBJS = $(patsubst $(KERNSRC)/%.c,$(BUILD)/%.o,$(wildcard $(KERNSRC)/*.c)) \
              $(patsubst $(KERNSRC)/%.S,$(BUILD)/%.o,$(wildcard $(KERNSRC)/*.S))
OBJS = $(ENTRY) $(KERNEL_OBJS)
KERNEL = $(BUILD)/kernel.elf

QEMU = qemu-system-i386
QEMUFLAGS = -hda $(IMG) -m 512 -monitor stdio -boot c -vga std -M pc

all: $(KERNEL) $(IMG)

# Make build directory
$(BUILD):
	mkdir -p $(BUILD)

# Assemble entry.S using GCC instead of AS
# This allows entry.S to resolve C-preprocessor macros and headers if needed
$(ENTRY): $(SRC)/entry.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile all C files
$(BUILD)/%.o: $(KERNSRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble other ASM files (.S) using GCC
$(BUILD)/%.o: $(KERNSRC)/%.S | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(IMG): $(KERNEL)
	dd if=/dev/zero of=$(IMG) bs=1M count=64
	parted -s $(IMG) mklabel msdos
	parted -s $(IMG) mkpart primary fat32 1MiB 100%
	parted -s $(IMG) set 1 boot on
	LOOP=$$(sudo losetup --find --partscan --show $(IMG)); \
	sudo mkfs.vfat -F 32 $${LOOP}p1; \
	mkdir -p mnt; \
	sudo mount $${LOOP}p1 mnt; \
	sudo mkdir -p mnt/boot; \
	sudo cp $(KERNEL) mnt/boot/kernel.elf; \
	sudo cp limine.cfg mnt/boot/limine.conf; \
	sudo cp test.txt mnt/test.txt; \
	sudo cp test.txt mnt/boot/test.txt; \
	sudo cp selftest mnt/boot/SELFTEST.SYS; \
	sudo cp limine/limine-bios.sys mnt/; \
	sync; \
	sudo umount mnt; \
	sudo losetup -d $$LOOP
	./limine/limine bios-install $(IMG)

# Run in QEMU
run: all
	$(QEMU) $(QEMUFLAGS)

# Clean
clean:
	sudo rm -rf $(BUILD) $(IMG) $(FATIMG) mnt *.ppm *.png
