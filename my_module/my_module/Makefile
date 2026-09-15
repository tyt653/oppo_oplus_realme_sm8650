obj-m += hello_world.o

all:
	make -C ../kernel_workspace/common M=$(PWD) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- CC=clang modules
