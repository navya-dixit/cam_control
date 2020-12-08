BINDIR = $(PWD)
SRC_DIR = $(PWD)
OBJ = $(SRC_DIR)/test_app.c

CC= $(CROSS_COMPILE)arm-poky-linux-gnueabi-gcc --sysroot=$(SDKTARGETSYSROOT)

TARGET = test_app.out

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(BINDIR)/$(TARGET) $(OBJ) -mfloat-abi=hard

.PHONY: clean
clean:
	rm -f ./src/*.o
	rm -f ./*.out
