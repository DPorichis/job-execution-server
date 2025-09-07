all: jobExecutorServer jobCommander progDelay

# paths
SOURCE = ./source
HEAD = ./include
TESTS = ./tests
BUILD = ./build
BIN = ./bin

CFLAGS = -g -I$(HEAD)

COMOBJ = $(BUILD)/jobCommander.o $(BUILD)/helpfunc.o $(BUILD)/utils.o
SEROBJ = $(BUILD)/jobExecutorServer.o $(BUILD)/helpfunc.o $(BUILD)/worker.o $(BUILD)/controller.o $(BUILD)/utils.o
TESOBJ = $(BUILD)/progDelay.o


$(BUILD) $(BIN):
	mkdir -p $@

$(BUILD)/%.o: $(SOURCE)/%.c | $(BUILD) $(BIN)
	gcc $(CFLAGS) -c $< -o $@

jobExecutorServer: $(SEROBJ)
	gcc $(SEROBJ) -o $(BIN)/jobExecutorServer -lpthread

jobCommander: $(COMOBJ)
	gcc $(COMOBJ) -o $(BIN)/jobCommander -lpthread

progDelay: $(TESOBJ)
	gcc $(TESOBJ) -o $(BIN)/progDelay

clean: 
	rm -rf $(BUILD) $(BIN)
