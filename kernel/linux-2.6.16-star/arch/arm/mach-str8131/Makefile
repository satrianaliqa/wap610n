#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y			:= str8131_debug.o str8131_setup.o str8131_timer.o str8131_intc.o str8131_misc.o str8131_counter.o str8131_pm.o 
obj-m			:=
obj-n			:=
obj-			:=

obj-$(CONFIG_PCI) += str8131_pci.o
obj-$(CONFIG_STR8131_DMA) += str8131_dma.o
obj-$(CONFIG_STR8131_HSDMA) += str8131_hsdma.o
obj-$(CONFIG_STR8131_I2S) += str8131_i2s.o

