PROJ_NAME = meinkraft
LIBS = -lglfw -lm
include master-makefile/Makefile
CFLAGS += -Wno-conversion -Wno-missing-braces