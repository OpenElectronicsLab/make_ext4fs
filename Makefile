# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2015 Jo-Philipp Wich <jow@openwrt.org>
# Modifications:
# Copyright (C) 2020 Hauke Mehrtens <hauke@hauke-m.de>
# Copyright (C) 2024 Eric Herman <eric@freesa.org>

CC ?= gcc
CFLAGS += -Isrc/include -Isrc/libsparse -Isrc/libsparse/include

BUILD_DIR ?= ./build

default: $(BUILD_DIR)/make_ext4fs

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT = indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0

ZLIB := -lz
ifeq ($(STATIC),1)
	ZLIB += -Wl,-Bstatic -Wl,-Bdynamic
endif

OBJ :=	\
	$(BUILD_DIR)/allocate.o \
	$(BUILD_DIR)/canned_fs_config.o \
	$(BUILD_DIR)/contents.o \
	$(BUILD_DIR)/crc16.o \
	$(BUILD_DIR)/ext4fixup.o \
	$(BUILD_DIR)/ext4_sb.o \
	$(BUILD_DIR)/ext4_utils.o \
	$(BUILD_DIR)/extent.o \
	$(BUILD_DIR)/indirect.o \
	$(BUILD_DIR)/make_ext4fs_main.o \
	$(BUILD_DIR)/make_ext4fs.o \
	$(BUILD_DIR)/sha1.o \
	$(BUILD_DIR)/uuid5.o \
	$(BUILD_DIR)/wipe.o

SPARSE_OBJ := \
	$(BUILD_DIR)/sparse/backed_block.o \
	$(BUILD_DIR)/sparse/output_file.o \
	$(BUILD_DIR)/sparse/sparse.o \
	$(BUILD_DIR)/sparse/sparse_crc32.o \
	$(BUILD_DIR)/sparse/sparse_err.o \
	$(BUILD_DIR)/sparse/sparse_read.o

$(BUILD_DIR)/sparse/%.o: src/libsparse/%.c
	mkdir -pv $(BUILD_DIR)/sparse
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILD_DIR)/%.o: src/%.c
	mkdir -pv $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILD_DIR)/make_ext4fs: $(OBJ) $(SPARSE_OBJ)
	echo "LD_FLAGS=$(LDFLAGS)"
	echo "ZLIB=$(ZLIB)"
	$(CC) $(LDFLAGS) -o $@ $^ $(ZLIB)

.PHONY:check-device
check-device: tests/build-and-test.sh $(BUILD_DIR)/make_ext4fs
	BUILD_DIR=$(BUILD_DIR) $<
	@echo SUCCESS $@

.PHONY: check-blockfile
check-blockfile: tests/build-and-test.sh $(BUILD_DIR)/make_ext4fs
	DIRECT_BLOCKFILE=1 BUILD_DIR=$(BUILD_DIR) $<
	@echo SUCCESS $@

.PHONY: check
check: check-device check-blockfile
	@echo SUCCESS $@

.PHONY: tidy
tidy:
	patch -Np1 -i misc/workaround-indent-bug-65165.patch
	$(LINDENT) \
		-T size_t -T ssize_t \
		-T uint8_t -T int8_t \
		-T uint16_t -T int16_t \
		-T uint32_t -T int32_t \
		-T uint64_t -T int64_t \
		-T FILE \
		-T jmp_buf \
		-T output_file \
		-T u8 -T u16 -T u32 -T u64 \
		-T u_int32_t \
		`find src -name '*.h' -o -name '*.c'`
	patch -Rp1 -i misc/workaround-indent-bug-65165.patch

.PHONY: clean
clean:
	rm -rfv $(OBJ) $(BUILD_DIR)/make_ext4fs $(BUILD_DIR)/*.o \
		$(BUILD_DIR)/sparse $(BUILD_DIR)/test-???? ./build
