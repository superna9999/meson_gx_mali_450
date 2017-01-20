Mali Utgard support for Amlogic Meson GX Family
===============================================

The Following SoCs are using the Mali-450 IP :
- Meson GXBB : S905
- Meson GXL : S905X and S905D

This distribution adds platform support for these families.

HowTo
=====

Apply the linux patches to upstream linux source and build.

```
$ KDIR=/path/to/amlogic/upstream/linux ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- ./build.sh
```

Copy mali.ko to root filesystem and boot.

Sources
=======

Original Mali kernel driver can be downloaded from : https://developer.arm.com/products/software/mali-drivers/utgard-kernel

The xf86-video-armsoc should be used in user space for X11 :
- https://github.com/superna9999/xf86-video-armsoc

along the r6p1-01rel0 libMali.so binary you can find at :
- http://openlinux.amlogic.com:8000/download/ARM/filesystem/arm-buildroot-2016-08-18-5aaca1b35f.tar.gz

In the buildroot/package/opengl/src/lib/arm64/r6p1/m450-X/ directory.

Place it into /usr/lib/mali/ and configured to fetch the EGL libraries by adding a file into :
```
/etc/ld.so.conf.d/mali.conf
```

containing :
```
/usr/lib/mali
```

And running :
```
# ldconfig
```

Mali libraries should have the following symlinks :
```
libEGL.so -> libEGL.so.1
libEGL.so.1 -> libEGL.so.1.4
libEGL.so.1.4 -> libMali.so
libGLESv1_CM.so -> libGLESv1_CM.so.1
libGLESv1_CM.so.1 -> libGLESv1_CM.so.1.1
libGLESv1_CM.so.1.1 -> libMali.so
libGLESv2.so -> libGLESv2.so.2
libGLESv2.so.2 -> libGLESv2.so.2.0
libGLESv2.so.2.0 -> libMali.so
libMali.so
```
