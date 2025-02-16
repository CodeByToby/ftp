CC = gcc
CFLAGS = -Wall -g

COMMON_DIR = ../Common
BUILD_DIR = build
OBJ_DIR = build/obj
SRC_DIR = .

OUT = $(BUILD_DIR)/server

SRC_ROOT = $(wildcard $(SRC_DIR)/*.c)
SRC_COMMON = $(wildcard $(COMMON_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_ROOT)) \
      $(patsubst $(COMMON_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_COMMON))
INCLUDE = -I$(COMMON_DIR)



all: $(OUT)

# Compiling phase
$(OUT): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Linking phase
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJ_DIR)/%.o: $(COMMON_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR)

clean:
	rm -f $(OBJ) $(OUT)