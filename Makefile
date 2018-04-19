# Makefile for JoSIM: Josephson Junction Superconductive SPICE Circuit Simulator

#Compiler, flags and things
CXX = g++ -std=c++17
CXX_FLAGS = -I. -I$(INCLUDE_DIR) -I$(INCLUDE_SUITESPARSE_DIR) -I$(INCLUDE_FL_DIR)
DEBUG_CXX_FLAGS = -g $(CXX_FLAGS)
PROGRAM_NAME = josim

# Directory Macros
CXX_SOURCE_DIR = src
CXX_OBJ_DIR = objs
INCLUDE_DIR = ./indclude
INCLUDE_SUITESPARSE_DIR = ./indclude/suitesparse
INCLUDE_FL_DIR = ./include/FL
BIN_DIR = bin

# Sources and Objects

CXX_SOURCE = $(shell find $(CXX_SOURCE_DIR)/ -name "*.cpp")
CXX_OBJ = $(CXX_SOURCE:$(CXX_SOURCE_DIR)/%.cpp=$(CXX_OBJ_DIR)/%.opp)
CXX_DOBJ = $(CXX_SOURC:$(CXX_SOURCE_DIR)/%.cpp=$(CXX_OBJ_DIR)/%.dopp)

BIN_PATH = $(BIN_DIR)/$(PROGRAM_NAME)

$(PROGRAM_NAME): $(CXX_OBJ)
	$(CXX) $(CXX_FLAGS) $(CXX_OBJ) -o $(BIN_PATH)

.SECONDEXPANSION:
$(CXX_OBJ): $$(patsubst $(CXX_OBJ_DIR)/%.opp,$(CXX_SOURCE_DIR)/%.cpp,$$@)
	$(CXX) -c $(CXX_FLAGS) -o "$@" "$<"

$(CXX_DOBJ): $$(patsubst $(CXX_OBJ_DIR)/%.dopp,$(CXX_SOURCE_DIR)/%.cpp,$$@)
	$(CXX) -c $(DEBUG_CXX_FLAGS) -o "$@" "$<"

.PHONY: debug
debug: $(CXX_DOBJ_PATH)
	$(CXX) -o josim.gdb "$^"

.PHONY: clean
clean:
	rm -f $(CXX_OBJ) $(BIN_PATH) josim.gdb

.PHONY: rebuild
rebuild:
	$(MAKE) clean
	$(MAKE) $(PROGRAM_NAME)
