TARGET = main

CC = gcc
CFLAGS = -Wall -Wextra `pkg-config --cflags dbus-1` -Iinc
LDFLAGS = `pkg-config --libs dbus-1`

SRC_DIR = src
BUILD_DIR = build

# File main ở root
MAIN_SRC = main.c
MAIN_OBJ = $(BUILD_DIR)/main.o

# Các file .c trong src/
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Luật mặc định
all: $(TARGET)

# Link tất cả .o
$(TARGET): $(MAIN_OBJ) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile main.c
$(BUILD_DIR)/main.o: $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile các file trong src/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Tạo thư mục build nếu chưa có
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Dọn dẹp file build
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

distclean: clean
	rm -f output/*

.PHONY: all clean distclean
