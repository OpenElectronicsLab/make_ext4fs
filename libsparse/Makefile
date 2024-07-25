CC ?= gcc
AR ?= ar
BUILD_DIR ?= ../build

all: $(BUILD_DIR)/libsparse.a

CFLAGS += -Iinclude

OBJ := \
	$(BUILD_DIR)/backed_block.o \
	$(BUILD_DIR)/output_file.o \
	$(BUILD_DIR)/sparse.o \
	$(BUILD_DIR)/sparse_crc32.o \
	$(BUILD_DIR)/sparse_err.o \
	$(BUILD_DIR)/sparse_read.o

$(BUILD_DIR)/%.o: %.c
	mkdir -pv $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $^

$(BUILD_DIR)/libsparse.a: $(OBJ)
	$(AR) rcs $@ $^

clean:
	rm -fv $(OBJ) $(BUILD_DIR)/libsparse.a
