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
# foreach
# https://www.gnu.org/software/make/manual/html_node/Foreach-Function.html
#
# patsubst : $(patsubst pattern,replacement,text)
#       https://www.gnu.org/software/make/manual/html_node/Text-Functions.html
#
# Target-specific Variable syntax:
# https://www.gnu.org/software/make/manual/html_node/Target_002dspecific.html


CC ?= gcc
BUILD_DIR ?= ./build
DEBUG_DIR ?= $(BUILD_DIR)/debug
COVER_DIR ?= $(BUILD_DIR)/cover

.PHONY: default debug coverage all
default: $(BUILD_DIR)/make_ext4fs
debug: $(DEBUG_DIR)/make_ext4fs
coverage: $(COVER_DIR)/make_ext4fs
all: default debug coverage

SHELL=/bin/bash

# -pedantic -Wc++-compat -Wcast-qual
COMMON_CFLAGS := -g -Wall -Wextra \
 -Isrc -Isrc/include -Isrc/libsparse -Isrc/libsparse/include \
 $(CFLAGS)

BUILD_CFLAGS=$(COMMON_CFLAGS)
DEBUG_CFLAGS=$(COMMON_CFLAGS) -Werror # -save-temps
COVER_CFLAGS=-O0 $(COMMON_CFLAGS) -Werror \
        -fno-inline-small-functions \
        -fkeep-inline-functions \
        -fkeep-static-functions \
        -fprofile-arcs \
        -ftest-coverage \
        --coverage

COVERAGE_LDFLAGS=--coverage

# extracted from https://github.com/torvalds/linux/blob/master/scripts/Lindent
LINDENT = indent -npro -kr -i8 -ts8 -sob -l80 -ss -ncs -cp1 -il0
# see also: https://www.kernel.org/doc/Documentation/process/coding-style.rst

ZLIB := -lz
ifeq ($(STATIC),1)
	ZLIB += -Wl,-Bstatic -Wl,-Bdynamic
endif
LDADD += $(ZLIB)

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
	output_file.o \
	sha1.o \
	sparse.o \
	sparse_crc32.o \
	sparse_err.o \
	sparse_read.o \
	uuid5.o \
	wipe.o

# Target-specific variable assignments
$(BUILD_DIR)/% : CURRENT_CFLAGS = $(BUILD_CFLAGS)
$(DEBUG_DIR)/% : CURRENT_CFLAGS = $(DEBUG_CFLAGS)
$(COVER_DIR)/% : CURRENT_CFLAGS = $(COVER_CFLAGS)

$(BUILD_DIR)/% : CURRENT_LDFLAGS = $(LDFLAGS)
$(DEBUG_DIR)/% : CURRENT_LDFLAGS = $(LDFLAGS)
$(COVER_DIR)/% : CURRENT_LDFLAGS = --coverage $(LDFLAGS)

%/:
	mkdir -pv $@

$(BUILD_DIR)/%.o: src/libsparse/%.c | $(BUILD_DIR)/
	$(CC) $(CURRENT_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)/
	$(CC) $(CURRENT_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/tests/test-%: tests/test-%.c \
		$(patsubst %, $(BUILD_DIR)/%, $(OBJ)) \
		| $(BUILD_DIR)/tests/
	$(CC) $(CURRENT_CFLAGS) $(LDFLAGS) -Itests -o $@ $^ $(LDADD)


$(DEBUG_DIR)/%.o: src/libsparse/%.c | $(DEBUG_DIR)/
	$(CC) $(CURRENT_CFLAGS) -c -o $@ $<

$(DEBUG_DIR)/%.o: src/%.c | $(DEBUG_DIR)/
	$(CC) $(CURRENT_CFLAGS) -c -o $@ $<

$(DEBUG_DIR)/tests/test-%: tests/test-%.c \
		$(patsubst %, $(DEBUG_DIR)/%, $(OBJ)) \
		| $(DEBUG_DIR)/tests/
	$(CC) $(CURRENT_CFLAGS) $(LDFLAGS) -Itests -o $@ $^ $(LDADD)


$(COVER_DIR)/%.o: src/libsparse/%.c | $(COVER_DIR)/
	$(CC) $(CURRENT_CFLAGS) -c -o $@ $<

$(COVER_DIR)/%.o: src/%.c | $(COVER_DIR)/
	$(CC) $(CURRENT_CFLAGS) -c -o $@ $<

$(COVER_DIR)/tests/test-%: tests/test-%.c \
		$(patsubst %, $(COVER_DIR)/%, $(OBJ)) \
		| $(COVER_DIR)/tests/
	$(CC) $(CURRENT_CFLAGS) $(LDFLAGS) -Itests -o $@ $^ $(LDADD)


%/make_ext4fs: %/make_ext4fs_main.o $(foreach obj,$(OBJ),%/$(obj)) | %/
	@echo "LD_FLAGS=$(LDFLAGS)"
	@echo "ZLIB=$(ZLIB)"
	$(CC) $(CURRENT_LDFLAGS) -o $@ $^ $(LDADD)

.PHONY:check-has-sudo
check-has-sudo:
	id | tr ',' '\n' | grep 'root\|sudo\|wheel' \
		|| id | tr ',' '\n'
	sudo env | grep -i sudo
	@echo SUCCESS $@

.PHONY:check-device
check-device: tests/build-and-test.sh $(BUILD_DIR)/make_ext4fs \
		check-has-sudo
	VERBOSE=1 \
		BUILD_DIR=$(BUILD_DIR) \
		DEBUG_DIR=$(DEBUG_DIR) \
		COVER_DIR=$(COVER_DIR) \
		$<
	@echo SUCCESS $@

.PHONY: check-blockfile
check-blockfile: tests/build-and-test.sh $(BUILD_DIR)/make_ext4fs \
		check-has-sudo
	VERBOSE=1 \
		DIRECT_BLOCKFILE=1 \
		BUILD_DIR=$(BUILD_DIR) \
		DEBUG_DIR=$(DEBUG_DIR) \
		COVER_DIR=$(COVER_DIR) \
		$<
	@echo SUCCESS $@


UNIT_TESTS= \
	test-uuid5-generate.c \
	test-uuid5.c

.PRECIOUS: \
	$(BUILD_DIR)/tests/test-uuid5 \
	$(BUILD_DIR)/tests/test-uuid5-generate \
	$(DEBUG_DIR)/tests/test-uuid5 \
	$(DEBUG_DIR)/tests/test-uuid5-generate \
	$(COVER_DIR)/tests/test-uuid5 \
	$(COVER_DIR)/tests/test-uuid5-generate


check-build-%: $(BUILD_DIR)/tests/test-%
	$<
	@echo SUCCESS $@

.PHONY: check-build-unit
check-build-unit: $(patsubst test-%.c, check-build-%, $(UNIT_TESTS))
	@echo SUCCESS $@


check-debug-%: $(DEBUG_DIR)/tests/test-%
	$<
	@echo SUCCESS $@

.PHONY: check-debug-unit
check-debug-unit: $(patsubst test-%.c, check-debug-%, $(UNIT_TESTS))
	@echo SUCCESS $@


check-cover-%: $(COVER_DIR)/tests/test-%
	$<
	@echo SUCCESS $@

.PHONY: check-cover-unit
check-cover-unit: $(patsubst test-%.c, check-cover-%, $(UNIT_TESTS))
	@echo SUCCESS $@

.PHONY: check
check: check-build-unit check-device check-blockfile
	@echo SUCCESS $@


$(COVER_DIR)/coverage.info: \
		check-cover-unit \
		tests/build-and-test.sh
	VERBOSE=1 \
		DIRECT_BLOCKFILE=1 \
		BUILD_DIR=$(BUILD_DIR) \
		DEBUG_DIR=$(DEBUG_DIR) \
		COVER_DIR=$(COVER_DIR) \
		BUILD_TYPE=cover \
		tests/build-and-test.sh
	lcov    --checksum \
                --capture \
                --base-directory . \
                --directory $(COVER_DIR) \
                --output-file $(COVER_DIR)/coverage.info
	@echo SUCCESS $@

$(COVER_DIR)/coverage_html/src/index.html: $(COVER_DIR)/coverage.info
	genhtml $< --output-directory $(COVER_DIR)/coverage_html
	@echo SUCCESS $@

.PHONY: coverage-report
coverage-report: \
		$(COVER_DIR)/coverage.info \
		$(COVER_DIR)/coverage_html/src/index.html
	ls -l $^
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
		-T CHAR64LONG16 \
		`find src tests -name '*.h' -o -name '*.c'`
	patch -Rp1 -i misc/workaround-indent-bug-65165.patch

.PHONY: clean
clean:
	rm -rfv $(BUILD_DIR)/* $(DEBUG_DIR)/* $(COVER_DIR)/*
