# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2015 Jo-Philipp Wich <jow@openwrt.org>
# Modifications:
# Copyright (C) 2020 Hauke Mehrtens <hauke@hauke-m.de>
# Copyright (C) 2024 Eric Herman <eric@freesa.org>

CC ?= gcc
CFLAGS += -Iinclude -Ilibsparse -Ilibsparse/include

BUILD_DIR ?= ./build

ifeq ($(STATIC),1)
	ZLIB := -Wl,-Bstatic -lz -Wl,-Bdynamic
else
	ZLIB := -lz
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

$(BUILD_DIR)/%.o: src/%.c
	mkdir -pv $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILD_DIR)/make_ext4fs: $(OBJ) $(BUILD_DIR)/libsparse.a
	$(CC) $(LDFLAGS) -o $@ $^ $(ZLIB)

$(BUILD_DIR)/libsparse.a:
	$(MAKE) -C libsparse/ BUILD_DIR=$(shell readlink -f $(BUILD_DIR))


.PHONY:check-device
check-device: tests/build-and-test.sh
	BUILD_DIR=$(BUILD_DIR) $<
	@echo SUCCESS $@

.PHONY: check-blockfile
check-blockfile: tests/build-and-test.sh
	DIRECT_BLOCKFILE=1 BUILD_DIR=$(BUILD_DIR) $<
	@echo SUCCESS $@

.PHONY: check
check: check-device check-blockfile
	@echo SUCCESS $@

.PHONY: clean
clean:
	$(MAKE) -C libsparse/ BUILD_DIR=$(shell readlink -f $(BUILD_DIR)) clean
	rm -rfv $(OBJ) $(BUILD_DIR)/make_ext4fs $(BUILD_DIR)/test-????
