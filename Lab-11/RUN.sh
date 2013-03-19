#!/bin/sh

set -x
qemu-system-arm \
	-M versatilepb -cpu arm926 \
	-nographic \
	-kernel kernel.elf
