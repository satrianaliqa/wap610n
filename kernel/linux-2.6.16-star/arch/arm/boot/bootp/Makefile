#
# linux/arch/arm/boot/bootp/Makefile
#

LDFLAGS_bootp	:=-p --no-undefined -X \
		 --defsym initrd_phys=$(INITRD_PHYS) \
		 --defsym kernel_phys=$(KERNEL_PHYS) \
		 --defsym params_phys=$(PARAMS_PHYS) -T
AFLAGS_initrd.o :=-DINITRD=\"$(INITRD)\"

targets	:= bootp init.o kernel.o initrd.o

# Note that bootp.lds picks up kernel.o and initrd.o
$(obj)/bootp:	$(src)/bootp.lds $(addprefix $(obj)/,init.o kernel.o initrd.o) FORCE
	echo start
	$(call if_changed,ld)
	@:
	echo done

# kernel.o and initrd.o includes a binary image using
# .incbin, a dependency which is not tracked automatically

$(obj)/kernel.o: arch/arm/boot/zImage FORCE

$(obj)/initrd.o: $(INITRD)  $(obj)/mkcsum FORCE

$(obj)/init.o:  FORCE

#$(obj)/cksum.S: arch/arm/boot/zImage $(INITRD) $(obj)/mkcsum
#	$(obj)/mkcsum  arch/arm/boot/zImage $(INITRD) > $(obj)/cksum.S

$(obj)/mkcsum: $(obj)/mkcsum.c
	$(HOSTCC) -O2 -o $(obj)/mkcsum $(obj)/mkcsum.c -lz

.PHONY:	$(INITRD) FORCE
