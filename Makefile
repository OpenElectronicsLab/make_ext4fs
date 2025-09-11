# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2015 Jo-Philipp Wich <jow@openwrt.org>
# Modifications:
# Copyright (C) 2020 Hauke Mehrtens <hauke@hauke-m.de>
# Copyright (C) 2024-2025 Eric Herman <eric@freesa.org>


# $@ : target label
# $< : the first prerequisite after the colon
# $^ : all of the prerequisite files
# $* : wildcard matched part
#
# https://www.gnu.org/software/make/manual/html_node/Prerequisite-Types.html
# targets : normal-prerequisites | order-only-prerequisites
#
# https://www.gnu.org/software/make/manual/html_node/Setting.html
# FOO = bar	# variables defined with ‘=’ are recursively expanded
# FOO := bar	# variables defined with ‘:=’ are simply expanded
# FOO :::= bar	# variables defined with ‘:::=’ are immediately expanded
# FOO ?= bar	# variable to be set to a value only if it’s not already set
#
# patsubst : $(patsubst pattern,replacement,text)
#       https://www.gnu.org/software/make/manual/html_node/Text-Functions.html


CC ?= gcc
# -pedantic -Wc++-compat -Wcast-qual
CFLAGS := -g -Wall -Wextra \
 -Isrc/include -Isrc/libsparse -Isrc/libsparse/include \
 $(CFLAGS)

BUILD_DIR ?= ./build

default: $(BUILD_DIR)/make_ext4fs

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT = indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0

ZLIB := -lz
ifeq ($(STATIC),1)
	ZLIB += -Wl,-Bstatic -Wl,-Bdynamic
endif

OBJ :=	\
	allocate.o \
	backed_block.o \
	canned_fs_config.o \
	contents.o \
	crc16.o \
	ext4_sb.o \
	ext4_utils.o \
	ext4fixup.o \
	extent.o \
	indirect.o \
	make_ext4fs.o \
	make_ext4fs_main.o \
	output_file.o \
	sha1.o \
	sparse.o \
	sparse_crc32.o \
	sparse_err.o \
	sparse_read.o \
	uuid5.o \
	wipe.o

$(BUILD_DIR):
	mkdir -pv $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/libsparse/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILD_DIR)/make_ext4fs: $(patsubst %, $(BUILD_DIR)/%, $(OBJ)) | $(BUILD_DIR)
	@echo "LD_FLAGS=$(LDFLAGS)"
	@echo "ZLIB=$(ZLIB)"
	$(CC) $(LDFLAGS) -o $@ $^ $(ZLIB)

.PHONY:check-has-sudo
check-has-sudo:
	id | tr ',' '\n' | grep 'root\|sudo\|wheel' \
		|| id | tr ',' '\n'
	sudo env | grep -i sudo
	@echo SUCCESS $@

.PHONY:check-device
check-device: tests/build-and-test.sh $(BUILD_DIR)/make_ext4fs \
		check-has-sudo
	BUILD_DIR=$(BUILD_DIR) $<
	@echo SUCCESS $@

.PHONY: check-blockfile
check-blockfile: tests/build-and-test.sh $(BUILD_DIR)/make_ext4fs \
		check-has-sudo
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
