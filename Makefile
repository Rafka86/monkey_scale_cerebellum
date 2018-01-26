DEFAULT_MAKE=/opt/pzsdk.ver4.0/make

CXX=mpic++

PZCL_KERNEL_DIRS = kernel.sc2
TARGET=main
CPPSRC=main.cpp
#CPPSRC+=pzclutil.cpp

LIBS += -lm

INC_DIR += -I./pezy

vpath %.cpp ./pezy

include $(DEFAULT_MAKE)/default_pzcl_host.mk
