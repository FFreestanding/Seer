OS_ARCH := x86
BUILD_DIR := build
KERNEL_DIR := kernel
OBJECT_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
ISO_DIR := $(BUILD_DIR)/iso
ISO_BOOT_DIR := $(ISO_DIR)/boot
ISO_GRUB_DIR := $(ISO_BOOT_DIR)/grub
INCLUDES_DIR := includes
INCLUDES := $(patsubst %, -I%, $(INCLUDES_DIR))

OS_NAME = Seer
OS_BIN = $(OS_NAME).bin
OS_ISO = $(OS_NAME).iso

CC := gcc
AS := as
LD := ld

O := -O0
W := -Wall -Wextra
CFLAGS := -m32 -fno-pie -g -std=gnu99 -ffreestanding $(O) $(W)
LDFLAGS := -m32 -z noexecstack -ffreestanding $(O) -nostdlib 
SOURCE_FILES := $(shell find -name "*.[cS]")
SRC := $(patsubst ./%, $(OBJECT_DIR)/%.o, $(SOURCE_FILES))
$(OBJECT_DIR):
	@mkdir -p $(OBJECT_DIR)
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)
$(ISO_DIR):
	@mkdir -p $(ISO_DIR)
	@mkdir -p $(ISO_BOOT_DIR)
	@mkdir -p $(ISO_GRUB_DIR)
$(OBJECT_DIR)/%.S.o: %.S
	@mkdir -p $(@D)
	$(CC) $(INCLUDES) -c $< -o $@ $(CFLAGS)
$(OBJECT_DIR)/%.c.o: %.c 
	@mkdir -p $(@D)
	$(CC) $(INCLUDES) -c $< -o $@ $(CFLAGS)
$(BIN_DIR)/$(OS_BIN): $(OBJECT_DIR) $(BIN_DIR) $(SRC)
	$(CC) -T linker.ld -o $(BIN_DIR)/$(OS_BIN) $(SRC) $(LDFLAGS)
	
$(BUILD_DIR)/$(OS_ISO): $(ISO_DIR) $(BIN_DIR)/$(OS_BIN) GRUB_TEMPLATE
	@python3 ./debug/generate_debug_info.py
	@./config-grub.sh ${OS_NAME} $(ISO_GRUB_DIR)/grub.cfg
	@cp $(BIN_DIR)/$(OS_BIN) $(ISO_BOOT_DIR)
	@grub-mkrescue -o $(BUILD_DIR)/$(OS_ISO) $(ISO_DIR)


all: clean $(BUILD_DIR)/$(OS_ISO)
all-debug: O := -O0
all-debug: CFLAGS := -m32 -fno-pie -g -std=gnu99 -ffreestanding $(O) $(W)
all-debug: LDFLAGS := -m32 -z noexecstack -ffreestanding $(O) -nostdlib 
all-debug: clean $(BUILD_DIR)/$(OS_ISO)
	@objdump -D $(BIN_DIR)/$(OS_BIN) > dump
	
clean:
	@rm -rf $(BUILD_DIR)
run: $(BUILD_DIR)/$(OS_ISO)
	qemu-system-i386 -d cpu_reset -cdrom $(BUILD_DIR)/$(OS_ISO) -no-reboot -no-shutdown

debug-qemu: all-debug
	@objcopy --only-keep-debug $(BIN_DIR)/$(OS_BIN) $(BUILD_DIR)/kernel.dbg
	@qemu-system-i386 -s -S -d cpu_reset -cdrom $(BUILD_DIR)/$(OS_ISO) -monitor telnet::45454,server,nowait  -no-shutdown -no-reboot &
	@gnome-terminal -- telnet 127.0.0.1 45454
	@qemu-system-i386 -s -S -kernel $(BIN_DIR)/$(OS_BIN) &
	@gdb -s $(BUILD_DIR)/kernel.dbg -ex "target remote localhost:1234"