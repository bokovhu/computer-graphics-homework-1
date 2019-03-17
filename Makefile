COMPILER = g++

SOURCES_DIR = src
LIBS_DIR = lib
BUILD_DIR = build

SOURCES = $(SOURCES_DIR)/framework.cpp $(SOURCES_DIR)/Skeleton.cpp
LINKER_FLAGS = -L$(LIBS_DIR) -lmingw32 -lfreeglut -lgdi32 -lglew32 -lopengl32
COMPILER_FLAGS = -std=c++11 -Wall -I$(LIBS_DIR)/include
EXEC_NAME = $(BUILD_DIR)/homework-1.exe

CMD_RMDIR = cmd /C rmdir
CMD_MKDIR = cmd /C mkdir
CMD_COPY = cmd /C copy

all: post-build
	echo "Done"
	
clean:
	$(CMD_RMDIR) /S /Q $(BUILD_DIR)
	
pre-build: clean
	$(CMD_MKDIR) $(BUILD_DIR)
	
build: pre-build
	$(COMPILER) -o $(EXEC_NAME) $(SOURCES) $(COMPILER_FLAGS) $(LINKER_FLAGS)
	
post-build: build
	$(CMD_COPY) /Y $(LIBS_DIR)\glew32.dll $(BUILD_DIR)
	$(CMD_COPY) /Y $(LIBS_DIR)\libfreeglut.dll $(BUILD_DIR)