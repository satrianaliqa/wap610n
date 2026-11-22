# WAP610N Kernel 2.6.16-star Customization & `mtlk.ko` Drop-In Hook

1. **ABI Compatibility Invariant**:
   - Do NOT modify core kernel headers (`include/linux/skbuff.h`, `include/linux/netdevice.h`, etc.) while `vendor/mtlk/driver/mtlk.ko` is a binary blob compiled against `2.6.16-star`.
2. **Building `mtlk.ko` From Source (When Available)**:
   - Run from repository root:
     ```bash
     make mtlk-driver MTLK_DRIVER_SRC=/path/to/mtlk/src
     ```
   - This compiles `mtlk.ko` against `kernel/linux-2.6.16-star` using `tools/arm-uclibc-3.4.6` and installs it directly to `vendor/mtlk/driver/mtlk.ko`.
