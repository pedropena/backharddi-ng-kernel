dir = lib/debian-installer.d

DEB_HOST_ARCH_CPU   := $(shell dpkg-architecture -qDEB_HOST_ARCH_CPU 2>/dev/null)
DEB_HOST_ARCH_OS    := $(shell dpkg-architecture -qDEB_HOST_ARCH_OS 2>/dev/null)

# Take account of old dpkg-architecture output.
ifeq ($(DEB_HOST_ARCH_CPU),)
  DEB_HOST_ARCH_CPU   := $(shell dpkg-architecture -qDEB_HOST_GNU_CPU)
  ifeq ($(DEB_HOST_ARCH_CPU),x86_64)
    DEB_HOST_ARCH_CPU   := amd64
  endif
endif
ifeq ($(DEB_HOST_ARCH_OS),)
  DEB_HOST_ARCH_OS    := $(shell dpkg-architecture -qDEB_HOST_GNU_SYSTEM)
endif

files = \
	S62directfb-hw-accel \
	S62directfb-left-handed

ifneq (,$(filter i386 amd64,$(DEB_HOST_ARCH_CPU)))
   files += \
	S59check-vesa-x86 \
	S61mouse-support-x86
endif
ifneq (,$(filter powerpc,$(DEB_HOST_ARCH_CPU)))
   files += \
	S61mouse-support-powerpc \
	S63directfb-linux-powerpc
endif

include ../../../Makefile.inc
