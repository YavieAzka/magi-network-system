# ============================================================
# Makefile — Magi System | IF2230 Jaringan Komputer
#
# LETAKKAN FILE INI DI ROOT PROJECT:
#   tugas-besar-jarkom-tamat_sudah/
#   ├── Makefile          <-- di sini
#   ├── src/
#   │   ├── main.cpp
#   │   ├── cli.cpp
#   │   ├── core/
#   │   ├── layer2/
#   │   ├── layer3/
#   │   ├── layer4/
#   │   └── utils/
#   └── include/
#       ├── core/
#       ├── layer2/
#       ├── layer3/
#       ├── layer4/
#       └── utils/
# ============================================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g
TARGET   := magi_system
BUILD    := build

# ROOT = direktori tempat Makefile ini berada (selalu benar
# terlepas dari mana make dipanggil)
ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

SRC     := $(ROOT)src
INC     := $(ROOT)include

# Include paths
INCLUDES := \
  -I$(SRC) \
  -I$(INC) \
  -I$(INC)/core \
  -I$(INC)/layer2 \
  -I$(INC)/layer3 \
  -I$(INC)/layer4 \
  -I$(INC)/utils

# Kumpulkan semua .cpp secara rekursif dari src/
SRCS := $(shell find $(SRC) -name "*.cpp")

OBJS := $(addprefix $(BUILD)/, $(notdir $(SRCS:.cpp=.o)))

# VPATH: daftar semua direktori yang mengandung .cpp
VPATH := $(sort $(dir $(SRCS)))

.PHONY: all run clean

all: $(BUILD) $(TARGET)

$(BUILD):
	mkdir -p $(BUILD)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo ""
	@echo "  Build berhasil -> ./$(TARGET)"
	@echo ""

$(BUILD)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Entry point wajib tugas besar
run: all
	./$(TARGET) $(ARGS)

clean:
	rm -rf $(BUILD) $(TARGET)
	@echo "  Artefak build dihapus."