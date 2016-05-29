TOOLCHAIN_BASE = /home/derf/var/projects/esp8266/toolchain/xtensa-lx106-elf/bin
SDK_BASE = /home/derf/var/projects/esp8266/toolchain/xtensa-lx106-elf/xtensa-lx106-elf/sysroot/usr
ESPTOOLPY = /home/derf/var/projects/esp8266/esptool/esptool.py
ESPTOOL = esptool
PORT = /dev/ttyUSB0

CC = ${TOOLCHAIN_BASE}/xtensa-lx106-elf-gcc
AR = ${TOOLCHAIN_BASE}/xtensa-lx106-elf-ar
LD = ${TOOLCHAIN_BASE}/xtensa-lx106-elf-gcc

CFLAGS = -std=c11 -Os
CFLAGS += -Wall -Wextra -Wno-implicit-function-declaration -Wno-unused-parameter -pedantic
CFLAGS += -Wl,-EL -fno-inline-functions -nostdlib
CFLAGS += -mlongcalls -mtext-section-literals
CFLAGS += -D__ets__ -DICACHE_FLASH -Isrc -I${SDK_BASE}/include
LDFLAGS = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

HEADERS  = $(wildcard src/*.h)
ASFILES  = $(wildcard src/*.S)
CFILES   = $(wildcard src/*.c)
CXXFILES = $(wildcard src/*.cc)
OBJECTS  = ${CFILES:src/%.c=build/%.o} ${CXXFILES:src/%.cc=build/%.o} ${ASFILES:src/%.S=build/%.o}

all: build build/0x00000.bin build/0x40000.bin

build:
	mkdir -p build

build/%.o: src/%.c
	${CC} ${CFLAGS} -c $< -o $@

build/main.elf: build/main.ar
	${CC} -L/${SDK_BASE}/lib -T${SDK_BASE}/lib/eagle.app.v6.ld ${LDFLAGS} -Wl,--start-group -lc -lgcc -lhal -lpp -lphy -lnet80211 -llwip -lwpa -lmain $< -Wl,--end-group -o $@

build/main.ar: ${OBJECTS}
	${AR} cru $@ $<


build/0x00000.bin: build/main.elf
	${ESPTOOL} -eo $< -bo $@  -bs .text -bs .data -bs .rodata -bc -ec

build/0x40000.bin: build/main.elf
	${ESPTOOL} -eo $< -es .irom0.text $@ -ec

build/%.o: src/%.c ${HEADERS}

program: flash

flash: build/0x00000.bin build/0x40000.bin
	${ESPTOOLPY} --port ${PORT} write_flash 0x00000 build/0x00000.bin 0x40000 build/0x40000.bin

clean:
	rm -rf build

.PHONY: all clean flash program
