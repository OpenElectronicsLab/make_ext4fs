CC ?= gcc
CFLAGS += -Iinclude -Ilibsparse/include

BUILD_DIR ?= .

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

$(BUILD_DIR)/%.o: %.c
	mkdir -pv $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILD_DIR)/make_ext4fs: $(OBJ) $(BUILD_DIR)/libsparse.a
	$(CC) $(LDFLAGS) -o $@ $^ $(ZLIB)

$(BUILD_DIR)/libsparse.a:
	$(MAKE) -C libsparse/ BUILD_DIR=$(shell readlink -f $(BUILD_DIR))


.PHONY:check-device
check-device:
	./build-and-test.sh
	@echo SUCCESS $@

.PHONY: check
check: check-device
	@echo SUCCESS $@

.PHONY: clean
clean:
	$(MAKE) -C libsparse/ BUILD_DIR=$(shell readlink -f $(BUILD_DIR)) clean
	rm -f $(OBJ) make_ext4fs
