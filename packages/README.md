# WAP610N Standardized `packages/` Plugin Framework

Every custom utility or lightweight daemon added for the WAP610N (4 MB Flash / 32 MB RAM) lives under `packages/<name>/` with a `Makefile.pkg` that includes `../Makefile.pkg`.

## Example `packages/mytool/Makefile.pkg`

```makefile
PKG_BIN         = mytool
PKG_OBJS        = mytool.o
PKG_INSTALL_DIR = usr/bin
PKG_CPPFLAGS    =
PKG_LDFLAGS     =

include ../Makefile.pkg
```

During `devscripts/net-ver-mkrootfs.sh` (or `make pack`), every `packages/*/Makefile.pkg` is compiled with `arm-linux-uclibc-gcc` (`-Os -std=gnu89`), stripped with `arm-linux-uclibc-strip`, and installed into `rootfs/`.
