#compiler
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -ggdb -pthread
OBJ = s-talk.o list.o
TARGET = s-talk
LDFLAG = -lm -pthread
#linker flag^ if needed
all: $(TARGET)

#run
run: $(TARGET)
	./$(TARGET)
	

#compile
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAG)
clean:
	$(RM) $(TARGET) $(OBJ) *~