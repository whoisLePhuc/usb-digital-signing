# Compiler & flags
CC       := gcc
CFLAGS   := -Wall -Wextra $(shell pkg-config --cflags dbus-1) -Iinc
LDFLAGS  := $(shell pkg-config --libs dbus-1) -lcrypto 

# Directories
SRC_DIR  := src
BUILD_DIR:= build

# Output executable
TARGET   := main

# Source & Object files
SRCS     := $(wildcard $(SRC_DIR)/*.c) main.c
OBJS     := $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile pattern rule
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
