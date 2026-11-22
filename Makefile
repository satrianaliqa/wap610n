# --------------------------------------------------------------------
# Unified WAP610N Build System (1 Board: STR8132, 1 Product: WAP610N)
# --------------------------------------------------------------------
.PHONY: all image configure build kernel apps webs pack verify docker-image docker-shell mtlk-driver clean

all: configure build

image: all

configure:
	./make_nv.sh reconf star-6.7.2-mtlk-U-Media-vela

build:
	./make_nv.sh build vendor/mtlk WAP610N

kernel:
	./make_nv.sh kernel

apps:
	./make_nv.sh apps

webs:
	./make_nv.sh webs

pack:
	./make_nv.sh pack

verify:
	./docker/verify-firmware.sh output/bootpImage

docker-image:
	./docker/compile.sh WAP610N

docker-shell:
	@docker run --rm -it --privileged -v "$(CURDIR):/workspace:z" -w /workspace \
		-e PATH="/workspace/tools/arm-uclibc-3.4.6/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
		wap610n-builder /bin/bash

mtlk-driver:
	@if [ -n "$(MTLK_DRIVER_SRC)" ] && [ -d "$(MTLK_DRIVER_SRC)" ]; then \
		echo "Building mtlk.ko from $(MTLK_DRIVER_SRC)..."; \
		PATH="$(CURDIR)/tools/arm-uclibc-3.4.6/bin:$$PATH" \
		$(MAKE) -C kernel/linux-2.6.16-star M="$(MTLK_DRIVER_SRC)" ARCH=arm CROSS_COMPILE=arm-linux-uclibc- modules && \
		cp -vf "$(MTLK_DRIVER_SRC)/mtlk.ko" vendor/mtlk/driver/mtlk.ko; \
	else \
		echo "Using prebuilt vendor/mtlk/driver/mtlk.ko (pass MTLK_DRIVER_SRC=/path/to/src to build from source)"; \
	fi

clean:
	./make_nv.sh clean WAP610N
