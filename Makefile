# paths
SOURCE = ./source
HEAD = ./include
TESTS = ./tests
BUILD = ./build
BIN = ./bin

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -g -I$(HEAD)

# Αρχεία .o
COMOBJ = $(BUILD)/jobCommander.o $(BUILD)/helpfunc.o $(BUILD)/utils.o
SEROBJ = $(BUILD)/jobExecutorServer.o $(BUILD)/helpfunc.o $(BUILD)/worker.o $(BUILD)/controller.o $(BUILD)/utils.o
TESOBJ = $(BUILD)/progDelay.o

$(BUILD)/%.o: $(SOURCE)/%.c
	gcc $(CFLAGS) -c $< -o $@


jobExecutorServer: $(SEROBJ)
	gcc $(SEROBJ) -o $(BIN)/jobExecutorServer -lpthread

jobCommander: $(COMOBJ)
	gcc $(COMOBJ) -o $(BIN)/jobCommander

all: jobExecutorServer jobCommander