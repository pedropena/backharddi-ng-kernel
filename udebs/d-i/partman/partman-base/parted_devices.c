#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

/* from <linux/cdrom.h> */
#define CDROM_GET_CAPABILITY	0x5331	/* get capabilities */
#endif /* __linux__ */

#include <parted/parted.h>

#ifdef __linux__
static int
is_cdrom(const char *path)
{
	int fd;
	int ret;

	fd = open(path, O_RDONLY | O_NONBLOCK);
	ret = ioctl(fd, CDROM_GET_CAPABILITY, NULL);
	close(fd);

	if (ret >= 0)
		return 1;
	else
		return 0;
}
#else /* !__linux__ */
#define is_cdrom(path) 0
#endif /* __linux__ */

#ifdef __linux__
static int
is_floppy(const char *path)
{
	return (strstr(path, "/dev/floppy") != NULL &&
		strstr(path, "/dev/fd") != NULL);
}
#else /* !__linux__ */
#define is_floppy(path) 0
#endif /* __linux__ */

int
main(int argc, char *argv[])
{
        PedDevice *dev;
        ped_exception_fetch_all();
        ped_device_probe_all();
        for (dev = NULL; NULL != (dev = ped_device_get_next(dev));) {
		if (dev->read_only)
			continue;
		if (is_cdrom(dev->path) || is_floppy(dev->path))
			continue;
                printf("%s\t%lli\t%s\n",
                       dev->path,
		       dev->length * PED_SECTOR_SIZE_DEFAULT,
                       dev->model);
        }
        return 0;
}

/*
Local variables:
indent-tabs-mode: nil
c-file-style: "linux"
c-font-lock-extra-types: ("Ped\\sw+")
End:
*/
