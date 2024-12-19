# Compiler
C = g++ -std=c++17

# Compiler flags
CFLAGS = -Wall -Wextra -pthread

# Executable name
TARGET = server

# Source and Object directories
SRC_DIR = src
OBJ_DIR = obj

# Source files
SRCS = $(SRC_DIR)/my_ftp.cpp $(SRC_DIR)/command_parser.cpp $(SRC_DIR)/server.cpp

# Object files
OBJS = $(OBJ_DIR)/my_ftp.o $(OBJ_DIR)/command_parser.o $(OBJ_DIR)/server.o

all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(C) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compiling
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR) 
	$(C) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)

.PHONY: all clean