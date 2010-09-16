include ../config

DIR := $(shell cd ..; pwd)
KERNEL := $(DIR)/boot/linux-$(KVERSION)-backharddi-ng
INITRD := $(DIR)/boot/minirt-$(KVERSION)-backharddi-ng.gz
CMDLINE := video=vesa:ywrap,mtrr vga=788 
DISK1_NAME := $(BUILDDIR)/disk1 
DISK1_SIZE := 100*2097152
DISK2_NAME := $(BUILDDIR)/disk2
DISK2_SIZE := 10*2097152
BACKUP_DISK_NAME := $(BUILDDIR)/disk2
BACKUP_DISK_SIZE := 10*2097152

.PHONY: test
test: 
	[ -f $(DISK1_NAME) ] || dd of=$(DISK1_NAME) seek=$$(($(DISK1_SIZE))) count=0
	[ -f $(DISK2_NAME) ] || dd of=$(DISK2_NAME) seek=$$(($(DISK2_SIZE))) count=0
	kvm -kernel $(KERNEL) -initrd $(INITRD) -append "$(CMDLINE)" -hda $(DISK1_NAME) -hdb $(DISK2_NAME)

.PHONY: tests
tests: auto_gen_void

auto_gen_void: del-backup-disk $(BACKUP_DISK_NAME)
	rm $(DISK1_NAME) || true
	dd of=$(DISK1_NAME) seek=$$(($(DISK1_SIZE))) count=0
	kvm -no-reboot -kernel $(KERNEL) -initrd $(INITRD) -append "$(CMDLINE) backharddi/medio=hd-media backharddi/modo=gen backharddi/imagenes=/target/test backharddi/reboot=" -hda $(DISK1_NAME) -hdb $(DISK2_NAME)

del-backup-disk:
	rm $(BACKUP_DISK_NAME) || true

$(BACKUP_DISK_NAME):
	if [ ! -f $(BACKUP_DISK_NAME) ]; then \
		dd of=$(BACKUP_DISK_NAME) seek=$$(($(BACKUP_DISK_SIZE))) count=0; \
		sfdisk -f $(BACKUP_DISK_NAME) < pt1; \
		sudo losetup -d /dev/loop0 || true; \
		sudo losetup -o $$((1*512)) --sizelimit $$((20971519*512)) /dev/loop0 $(BACKUP_DISK_NAME); \
		sudo mkfs.ext3 -L backharddi /dev/loop0; \
		sudo losetup -d /dev/loop0; \
	fi
