/**
 * ntfsmount - Part of the Linux-NTFS project.
 *
 * Copyright (c) 2005-2006 Yura Pakhuchiy
 * Copyright (c) 2005 Yuval Fledel
 *
 * NTFS module for FUSE.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <fuse.h>
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <signal.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <getopt.h>
#include <syslog.h>

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "attrib.h"
#include "inode.h"
#include "volume.h"
#include "dir.h"
#include "unistr.h"
#include "layout.h"
#include "index.h"
#include "utils.h"
#include "version.h"
#include "ntfstime.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
	fuse_fill_dir_t filler;
	void *buf;
} ntfs_fuse_fill_context_t;

typedef enum {
	NF_STREAMS_INTERFACE_NONE,	/* No access to named data streams. */
	NF_STREAMS_INTERFACE_XATTR,	/* Map named data streams to xattrs. */
	NF_STREAMS_INTERFACE_WINDOWS,	/* "file:stream" interface. */
} ntfs_fuse_streams_interface;

typedef struct {
	ntfs_volume *vol;
	int state;
	long free_clusters;
	long free_mft;
	unsigned int uid;
	unsigned int gid;
	unsigned int fmask;
	unsigned int dmask;
	ntfs_fuse_streams_interface streams;
	BOOL ro;
	BOOL show_sys_files;
	BOOL silent;
	BOOL force;
	BOOL debug;
	BOOL noatime;
	BOOL no_detach;
} ntfs_fuse_context_t;

typedef enum {
	NF_FreeClustersOutdate	= (1 << 0),  /* Information about amount of
						free clusters is outdated. */
	NF_FreeMFTOutdate	= (1 << 1),  /* Information about amount of
						free MFT records is outdated. */
} ntfs_fuse_state_bits;

static struct options {
	char	*mnt_point;	/* Mount point */
	char	*options;	/* Mount options */
	char	*device;	/* Device to mount */
	int	 quiet;		/* Less output */
	int	 verbose;	/* Extra output */
} opts;

static const char *EXEC_NAME = "ntfsmount";
static char def_opts[] = "default_permissions,allow_other,";
static ntfs_fuse_context_t *ctx;

static __inline__ void ntfs_fuse_mark_free_space_outdated(void)
{
	/* Mark information about free MFT record and clusters outdated. */
	ctx->state |= (NF_FreeClustersOutdate | NF_FreeMFTOutdate);
}

/**
 * ntfs_fuse_is_named_data_stream - check path to be to named data stream
 * @path:	path to check
 *
 * Returns 1 if path is to named data stream or 0 otherwise.
 */
static __inline__ int ntfs_fuse_is_named_data_stream(const char *path)
{
	if (strchr(path, ':') && ctx->streams == NF_STREAMS_INTERFACE_WINDOWS)
		return 1;
	return 0;
}

static long ntfs_fuse_get_nr_free_mft_records(ntfs_volume *vol)
{
	u8 *buf;
	long nr_free = 0;
	s64 br, total = 0;

	if (!(ctx->state & NF_FreeMFTOutdate))
		return ctx->free_mft;
	buf = malloc(vol->cluster_size);
	if (!buf)
		return -ENOMEM;
	while (1) {
		int i, j;

		br = ntfs_attr_pread(vol->mftbmp_na, total,
				vol->cluster_size, buf);
		if (!br)
			break;
		total += br;
		for (i = 0; i < br; i++)
			for (j = 0; j < 8; j++)
				if (!((buf[i] >> j) & 1))
					nr_free++;
	}
	free(buf);
	if (!total)
		return -errno;
	ctx->free_mft = nr_free;
	ctx->state &= ~(NF_FreeMFTOutdate);
	return nr_free;
}

static long ntfs_fuse_get_nr_free_clusters(ntfs_volume *vol)
{
	u8 *buf;
	long nr_free = 0;
	s64 br, total = 0;

	if (!(ctx->state & NF_FreeClustersOutdate))
		return ctx->free_clusters;
	buf = malloc(vol->cluster_size);
	if (!buf)
		return -ENOMEM;
	while (1) {
		int i, j;

		br = ntfs_attr_pread(vol->lcnbmp_na, total,
				vol->cluster_size, buf);
		if (!br)
			break;
		total += br;
		for (i = 0; i < br; i++)
			for (j = 0; j < 8; j++)
				if (!((buf[i] >> j) & 1))
					nr_free++;
	}
	free(buf);
	if (!total)
		return -errno;
	ctx->free_clusters = nr_free;
	ctx->state &= ~(NF_FreeClustersOutdate);
	return nr_free;
}

/**
 * ntfs_fuse_statfs - return information about mounted NTFS volume
 * @path:	ignored (but fuse requires it)
 * @sfs:	statfs structure in which to return the information
 *
 * Return information about the mounted NTFS volume @sb in the statfs structure
 * pointed to by @sfs (this is initialized with zeros before ntfs_statfs is
 * called). We interpret the values to be correct of the moment in time at
 * which we are called. Most values are variable otherwise and this isn't just
 * the free values but the totals as well. For example we can increase the
 * total number of file nodes if we run out and we can keep doing this until
 * there is no more space on the volume left at all.
 *
 * This code based on ntfs_statfs from ntfs kernel driver.
 *
 * Returns 0 on success or -errno on error.
 */
static int ntfs_fuse_statfs(const char *path __attribute__((unused)),
#if defined(FUSE_VERSION) && (FUSE_VERSION >= 25)
		struct statvfs *sfs)
#else
		struct statfs *sfs)
#endif
{
	long size;
	ntfs_volume *vol;

	vol = ctx->vol;
	if (!vol)
		return -ENODEV;
	/* Optimal transfer block size. */
	sfs->f_bsize = vol->cluster_size;
#if defined(FUSE_VERSION) && (FUSE_VERSION >= 25)
	sfs->f_frsize = vol->cluster_size;
#endif
	/*
	 * Total data blocks in file system in units of f_bsize and since
	 * inodes are also stored in data blocs ($MFT is a file) this is just
	 * the total clusters.
	 */
	sfs->f_blocks = vol->nr_clusters;
	/* Free data blocks in file system in units of f_bsize. */
	size = ntfs_fuse_get_nr_free_clusters(vol);
	if (size < 0)
		size = 0;
	/* Free blocks avail to non-superuser, same as above on NTFS. */
	sfs->f_bavail = sfs->f_bfree = size;
	/* Number of inodes in file system (at this point in time). */
	sfs->f_files = vol->mft_na->data_size >> vol->mft_record_size_bits;
	/* Free inodes in fs (based on current total count). */
	size = ntfs_fuse_get_nr_free_mft_records(vol);
	if (size < 0)
		size = 0;
	sfs->f_ffree = size;
	/* Maximum length of filenames. */
#if defined(FUSE_VERSION) && (FUSE_VERSION >= 25)
	sfs->f_namemax = NTFS_MAX_NAME_LEN;
#else
	sfs->f_namelen = NTFS_MAX_NAME_LEN;
#endif
	return 0;
}

/**
 * ntfs_fuse_parse_path - split path to path and stream name.
 * @org_path:		path to split
 * @path:		pointer to buffer in which parsed path saved
 * @stream_name:	pointer to buffer where stream name in unicode saved
 *
 * This function allocates buffers for @*path and @*stream, user must free them
 * after use.
 *
 * Return values:
 *	<0	Error occurred, return -errno;
 *	 0	No stream name, @*stream is not allocated and set to AT_UNNAMED.
 *	>0	Stream name length in unicode characters.
 */
static int ntfs_fuse_parse_path(const char *org_path, char **path,
		ntfschar **stream_name)
{
	char *stream_name_mbs;
	int res;

	stream_name_mbs = strdup(org_path);
	if (!stream_name_mbs)
		return -errno;
	if (ctx->streams == NF_STREAMS_INTERFACE_WINDOWS) {
		*path = strsep(&stream_name_mbs, ":");
		if (stream_name_mbs) {
			*stream_name = NULL;
			res = ntfs_mbstoucs(stream_name_mbs, stream_name, 0);
			if (res < 0)
				return -errno;
			return res;
		}
	} else
		*path = stream_name_mbs;
	*stream_name = AT_UNNAMED;
	return 0;
}

static int ntfs_fuse_getattr(const char *org_path, struct stat *stbuf)
{
	int res = 0;
	ntfs_inode *ni;
	ntfs_attr *na;
	ntfs_volume *vol;
	char *path = NULL;
	ntfschar *stream_name;
	int stream_name_len;

	vol = ctx->vol;
	stream_name_len = ntfs_fuse_parse_path(org_path, &path, &stream_name);
	if (stream_name_len < 0)
		return stream_name_len;
	memset(stbuf, 0, sizeof(struct stat));
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni) {
		res = -ENOENT;
		goto exit;
	}
	if (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY && !stream_name_len) {
		/* Directory. */
		stbuf->st_mode = S_IFDIR | (0777 & ~ctx->dmask);
		na = ntfs_attr_open(ni, AT_INDEX_ALLOCATION, NTFS_INDEX_I30, 4);
		if (na) {
			stbuf->st_size = na->data_size;
			stbuf->st_blocks = na->allocated_size >>
				vol->sector_size_bits;
			ntfs_attr_close(na);
		} else {
			stbuf->st_size = 0;
			stbuf->st_blocks = 0;
		}
		stbuf->st_nlink = 1; /* Needed for correct find work. */
	} else {
		/* Regular or Interix (INTX) file. */
		stbuf->st_mode = S_IFREG;
		stbuf->st_size = ni->data_size;
		stbuf->st_blocks = ni->allocated_size >> vol->sector_size_bits;
		stbuf->st_nlink = le16_to_cpu(ni->mrec->link_count);
		if (ni->flags & FILE_ATTR_SYSTEM || stream_name_len) {
			na = ntfs_attr_open(ni, AT_DATA, stream_name,
					stream_name_len);
			if (!na) {
				if (stream_name_len)
					res = -ENOENT;
				goto exit;
			}
			if (stream_name_len) {
				stbuf->st_size = na->data_size;
				stbuf->st_blocks = na->allocated_size >>
					vol->sector_size_bits;
			}
			/* Check whether it's Interix FIFO or socket. */
			if (!(ni->flags & FILE_ATTR_HIDDEN) &&
					!stream_name_len) {
				/* FIFO. */
				if (na->data_size == 0)
					stbuf->st_mode = S_IFIFO;
				/* Socket link. */
				if (na->data_size == 1)
					stbuf->st_mode = S_IFSOCK;
			}
			/*
			 * Check whether it's Interix symbolic link, block or
			 * character device.
			 */
			if (na->data_size <= sizeof(INTX_FILE_TYPES) + sizeof(
					ntfschar) * MAX_PATH && na->data_size >
					sizeof(INTX_FILE_TYPES) &&
					!stream_name_len) {
				INTX_FILE *intx_file;

				intx_file = malloc(na->data_size);
				if (!intx_file) {
					res = -errno;
					ntfs_attr_close(na);
					goto exit;
				}
				if (ntfs_attr_pread(na, 0, na->data_size,
						intx_file) != na->data_size) {
					res = -errno;
					free(intx_file);
					ntfs_attr_close(na);
					goto exit;
				}
				if (intx_file->magic == INTX_BLOCK_DEVICE &&
						na->data_size == offsetof(
						INTX_FILE, device_end)) {
					stbuf->st_mode = S_IFBLK;
					stbuf->st_rdev = makedev(le64_to_cpu(
							intx_file->major),
							le64_to_cpu(
							intx_file->minor));
				}
				if (intx_file->magic == INTX_CHARACTER_DEVICE &&
						na->data_size == offsetof(
						INTX_FILE, device_end)) {
					stbuf->st_mode = S_IFCHR;
					stbuf->st_rdev = makedev(le64_to_cpu(
							intx_file->major),
							le64_to_cpu(
							intx_file->minor));
				}
				if (intx_file->magic == INTX_SYMBOLIC_LINK)
					stbuf->st_mode = S_IFLNK;
				free(intx_file);
			}
			ntfs_attr_close(na);
		}
		stbuf->st_mode |= (0777 & ~ctx->fmask);
	}
	stbuf->st_uid = ctx->uid;
	stbuf->st_gid = ctx->gid;
	stbuf->st_ino = ni->mft_no;
	stbuf->st_atime = ni->last_access_time;
	stbuf->st_ctime = ni->last_mft_change_time;
	stbuf->st_mtime = ni->last_data_change_time;
exit:
	if (ni)
		ntfs_inode_close(ni);
	free(path);
	if (stream_name_len)
		free(stream_name);
	return res;
}

static int ntfs_fuse_readlink(const char *org_path, char *buf, size_t buf_size)
{
	char *path;
	ntfschar *stream_name;
	ntfs_inode *ni = NULL;
	ntfs_attr *na = NULL;
	INTX_FILE *intx_file = NULL;
	int stream_name_len, res = 0;

	/* Get inode. */
	stream_name_len = ntfs_fuse_parse_path(org_path, &path, &stream_name);
	if (stream_name_len < 0)
		return stream_name_len;
	if (stream_name_len > 0) {
		res = -EINVAL;
		goto exit;
	}
	ni = ntfs_pathname_to_inode(ctx->vol, NULL, path);
	if (!ni) {
		res = -errno;
		goto exit;
	}
	/* Sanity checks. */
	if (!(ni->flags & FILE_ATTR_SYSTEM)) {
		res = -EINVAL;
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, AT_UNNAMED, 0);
	if (!na) {
		res = -errno;
		goto exit;
	}
	if (na->data_size <= sizeof(INTX_FILE_TYPES)) {
		res = -EINVAL;
		goto exit;
	}
	if (na->data_size > sizeof(INTX_FILE_TYPES) +
			sizeof(ntfschar) * MAX_PATH) {
		res = -ENAMETOOLONG;
		goto exit;
	}
	/* Receive file content. */
	intx_file = malloc(na->data_size);
	if (!intx_file) {
		res = -errno;
		goto exit;
	}
	if (ntfs_attr_pread(na, 0, na->data_size, intx_file) != na->data_size) {
		res = -errno;
		goto exit;
	}
	/* Sanity check. */
	if (intx_file->magic != INTX_SYMBOLIC_LINK) {
		res = -EINVAL;
		goto exit;
	}
	/* Convert link from unicode to local encoding. */
	if (ntfs_ucstombs(intx_file->target, (na->data_size -
			offsetof(INTX_FILE, target)) / sizeof(ntfschar),
			&buf, buf_size) < 0) {
		res = -errno;
		goto exit;
	}
exit:
	if (intx_file)
		free(intx_file);
	if (na)
		ntfs_attr_close(na);
	if (ni)
		ntfs_inode_close(ni);
	free(path);
	if (stream_name_len)
		free(stream_name);
	return res;
}

static int ntfs_fuse_filler(ntfs_fuse_fill_context_t *fill_ctx,
		const ntfschar *name, const int name_len, const int name_type,
		const s64 pos __attribute__((unused)), const MFT_REF mref,
		const unsigned dt_type __attribute__((unused)))
{
	char *filename = NULL;

	if (name_type == FILE_NAME_DOS)
		return 0;
	if (ntfs_ucstombs(name, name_len, &filename, 0) < 0) {
		ntfs_log_error("Skipping unrepresentable filename (inode %lld):"
				" %s\n", MREF(mref), strerror(errno));
		return 0;
	}
	if (ntfs_fuse_is_named_data_stream(filename)) {
		ntfs_log_error("Unable to access '%s' (inode %lld) with "
				"current named streams access interface.\n",
				filename, MREF(mref));
		free(filename);
		return 0;
	}
	if (MREF(mref) == FILE_root || MREF(mref) >= FILE_first_user ||
			ctx->show_sys_files)
		fill_ctx->filler(fill_ctx->buf, filename, NULL, 0);
	free(filename);
	return 0;
}

static int ntfs_fuse_readdir(const char *path, void *buf,
		fuse_fill_dir_t filler, off_t offset __attribute__((unused)),
		struct fuse_file_info *fi __attribute__((unused)))
{
	ntfs_fuse_fill_context_t fill_ctx;
	ntfs_volume *vol;
	ntfs_inode *ni;
	s64 pos = 0;
	int err = 0;

	vol = ctx->vol;
	fill_ctx.filler = filler;
	fill_ctx.buf = buf;
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni)
		return -errno;
	if (ntfs_readdir(ni, &pos, &fill_ctx,
			(ntfs_filldir_t)ntfs_fuse_filler))
		err = -errno;
	ntfs_inode_close(ni);
	return err;
}

static int ntfs_fuse_open(const char *org_path,
		struct fuse_file_info *fi __attribute__((unused)))
{
	ntfs_volume *vol;
	ntfs_inode *ni;
	ntfs_attr *na;
	int res = 0;
	char *path = NULL;
	ntfschar *stream_name;
	int stream_name_len;

	stream_name_len = ntfs_fuse_parse_path(org_path, &path, &stream_name);
	if (stream_name_len < 0)
		return stream_name_len;
	vol = ctx->vol;
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (ni) {
		na = ntfs_attr_open(ni, AT_DATA, stream_name, stream_name_len);
		if (na) {
			if (NAttrEncrypted(na))
				res = -EACCES;
			ntfs_attr_close(na);
		} else
			res = -errno;
		ntfs_inode_close(ni);
	} else
		res = -errno;
	free(path);
	if (stream_name_len)
		free(stream_name);
	return res;
}

static int ntfs_fuse_read(const char *org_path, char *buf, size_t size,
		off_t offset, struct fuse_file_info *fi __attribute__((unused)))
{
	ntfs_volume *vol;
	ntfs_inode *ni = NULL;
	ntfs_attr *na = NULL;
	char *path = NULL;
	ntfschar *stream_name;
	int stream_name_len, res, total = 0;

	stream_name_len = ntfs_fuse_parse_path(org_path, &path, &stream_name);
	if (stream_name_len < 0)
		return stream_name_len;
	vol = ctx->vol;
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni) {
		res = -errno;
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, stream_name, stream_name_len);
	if (!na) {
		res = -errno;
		goto exit;
	}
	if (offset + size > na->data_size)
		size = na->data_size - offset;
	while (size) {
		res = ntfs_attr_pread(na, offset, size, buf);
		if (res < (s64)size)
			ntfs_log_error("ntfs_attr_pread returned less bytes "
					"than requested.\n");
		if (res <= 0) {
			res = -errno;
			goto exit;
		}
		size -= res;
		offset += res;
		total += res;
	}
	res = total;
exit:
	if (na)
		ntfs_attr_close(na);
	if (ni && ntfs_inode_close(ni))
		ntfs_log_perror("Failed to close inode");
	free(path);
	if (stream_name_len)
		free(stream_name);
	return res;
}

static int ntfs_fuse_write(const char *org_path, const char *buf, size_t size,
		off_t offset, struct fuse_file_info *fi __attribute__((unused)))
{
	ntfs_volume *vol;
	ntfs_inode *ni = NULL;
	ntfs_attr *na = NULL;
	char *path = NULL;
	ntfschar *stream_name;
	int stream_name_len, res, total = 0;

	stream_name_len = ntfs_fuse_parse_path(org_path, &path, &stream_name);
	if (stream_name_len < 0)
		return stream_name_len;
	vol = ctx->vol;
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni) {
		res = -errno;
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, stream_name, stream_name_len);
	if (!na) {
		res = -errno;
		goto exit;
	}
	while (size) {
		res = ntfs_attr_pwrite(na, offset, size, buf);
		if (res < (s64)size)
			ntfs_log_error("ntfs_attr_pwrite returned less bytes "
					"than requested.\n");
		if (res <= 0) {
			res = -errno;
			goto exit;
		}
		size -= res;
		offset += res;
		total += res;
	}
	res = total;
exit:
	ntfs_fuse_mark_free_space_outdated();
	if (na)
		ntfs_attr_close(na);
	if (ni && ntfs_inode_close(ni))
		ntfs_log_perror("Failed to close inode");
	free(path);
	if (stream_name_len)
		free(stream_name);
	return res;
}

static int ntfs_fuse_truncate(const char *org_path, off_t size)
{
	ntfs_volume *vol;
	ntfs_inode *ni = NULL;
	ntfs_attr *na;
	int res;
	char *path = NULL;
	ntfschar *stream_name;
	int stream_name_len;

	stream_name_len = ntfs_fuse_parse_path(org_path, &path, &stream_name);
	if (stream_name_len < 0)
		return stream_name_len;
	vol = ctx->vol;
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni) {
		res = -errno;
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, stream_name, stream_name_len);
	if (!na) {
		res = -errno;
		goto exit;
	}
	res = ntfs_attr_truncate(na, size);
	ntfs_fuse_mark_free_space_outdated();
	ntfs_attr_close(na);
exit:
	if (ni && ntfs_inode_close(ni))
		ntfs_log_perror("Failed to close inode");
	free(path);
	if (stream_name_len)
		free(stream_name);
	return res;
}

static int ntfs_fuse_chmod(const char *path,
		mode_t mode __attribute__((unused)))
{
	if (ntfs_fuse_is_named_data_stream(path))
		return -EINVAL; /* n/a for named data streams. */
	if (ctx->silent)
		return 0;
	return -EOPNOTSUPP;
}

static int ntfs_fuse_chown(const char *path, uid_t uid __attribute__((unused)),
		gid_t gid __attribute__((unused)))
{
	if (ntfs_fuse_is_named_data_stream(path))
		return -EINVAL; /* n/a for named data streams. */
	if (ctx->silent)
		return 0;
	return -EOPNOTSUPP;
}

static int ntfs_fuse_create(const char *org_path, dev_t type, dev_t dev,
		const char *target)
{
	char *name;
	ntfschar *uname = NULL, *utarget = NULL;
	ntfs_inode *dir_ni = NULL, *ni;
	char *path;
	int res = 0, uname_len, utarget_len;

	path = strdup(org_path);
	if (!path)
		return -errno;
	/* Generate unicode filename. */
	name = strrchr(path, '/');
	name++;
	uname_len = ntfs_mbstoucs(name, &uname, 0);
	if (uname_len < 0) {
		res = -errno;
		goto exit;
	}
	/* Open parent directory. */
	*name = 0;
	dir_ni = ntfs_pathname_to_inode(ctx->vol, NULL, path);
	if (!dir_ni) {
		res = -errno;
		if (res == -ENOENT)
			res = -EIO;
		goto exit;
	}
	/* Create object specified in @type. */
	switch (type) {
		case S_IFCHR:
		case S_IFBLK:
			ni = ntfs_create_device(dir_ni, uname, uname_len, type,
					dev);
			break;
		case S_IFLNK:
			utarget_len = ntfs_mbstoucs(target, &utarget, 0);
			if (utarget_len < 0) {
				res = -errno;
				goto exit;
			}
			ni = ntfs_create_symlink(dir_ni, uname, uname_len,
					utarget, utarget_len);
			break;
		default:
			ni = ntfs_create(dir_ni, uname, uname_len, type);
			break;
	}
	if (ni)
		ntfs_inode_close(ni);
	else
		res = -errno;
exit:
	free(uname);
	if (dir_ni)
		ntfs_inode_close(dir_ni);
	if (utarget)
		free(utarget);
	free(path);
	return res;
}

static int ntfs_fuse_create_stream(const char *path,
		ntfschar *stream_name, const int stream_name_len)
{
	ntfs_inode *ni;
	int res = 0;

	ni = ntfs_pathname_to_inode(ctx->vol, NULL, path);
	if (!ni) {
		res = -errno;
		if (res == -ENOENT) {
			/*
			 * If such file does not exist, create it and try once
			 * again to add stream to it.
			 */
			res = ntfs_fuse_create(path, S_IFREG, 0, NULL);
			if (!res)
				return ntfs_fuse_create_stream(path,
						stream_name, stream_name_len);
			else
				res = -errno;
		}
		return res;
	}
	if (ntfs_attr_add(ni, AT_DATA, stream_name, stream_name_len, NULL, 0))
		res = -errno;
	if (ntfs_inode_close(ni))
		ntfs_log_perror("Failed to close inode");
	return res;
}

static int ntfs_fuse_mknod(const char *org_path, mode_t mode, dev_t dev)
{
	char *path = NULL;
	ntfschar *stream_name;
	int stream_name_len;
	int res = 0;

	stream_name_len = ntfs_fuse_parse_path(org_path, &path, &stream_name);
	if (stream_name_len < 0)
		return stream_name_len;
	if (stream_name_len && !S_ISREG(mode)) {
		res = -EINVAL;
		goto exit;
	}
	if (!stream_name_len)
		res = ntfs_fuse_create(path, mode & S_IFMT, dev, NULL);
	else
		res = ntfs_fuse_create_stream(path, stream_name,
				stream_name_len);
	ntfs_fuse_mark_free_space_outdated();
exit:
	free(path);
	if (stream_name_len)
		free(stream_name);
	return res;
}

static int ntfs_fuse_symlink(const char *to, const char *from)
{
	if (ntfs_fuse_is_named_data_stream(from))
		return -EINVAL; /* n/a for named data streams. */
	ntfs_fuse_mark_free_space_outdated();
	return ntfs_fuse_create(from, S_IFLNK, 0, to);
}

static int ntfs_fuse_link(const char *old_path, const char *new_path)
{
	char *name;
	ntfschar *uname = NULL;
	ntfs_inode *dir_ni = NULL, *ni;
	char *path;
	int res = 0, uname_len;

	if (ntfs_fuse_is_named_data_stream(old_path))
		return -EINVAL; /* n/a for named data streams. */
	if (ntfs_fuse_is_named_data_stream(new_path))
		return -EINVAL; /* n/a for named data streams. */
	path = strdup(new_path);
	if (!path)
		return -errno;
	/* Open file for which create hard link. */
	ni = ntfs_pathname_to_inode(ctx->vol, NULL, old_path);
	if (!ni) {
		res = -errno;
		goto exit;
	}
	/* Generate unicode filename. */
	name = strrchr(path, '/');
	name++;
	uname_len = ntfs_mbstoucs(name, &uname, 0);
	if (uname_len < 0) {
		res = -errno;
		goto exit;
	}
	/* Open parent directory. */
	*name = 0;
	dir_ni = ntfs_pathname_to_inode(ctx->vol, NULL, path);
	if (!dir_ni) {
		res = -errno;
		if (res == -ENOENT)
			res = -EIO;
		goto exit;
	}
	ntfs_fuse_mark_free_space_outdated();
	/* Create hard link. */
	if (ntfs_link(ni, dir_ni, uname, uname_len))
		res = -errno;
exit:
	if (ni)
		ntfs_inode_close(ni);
	free(uname);
	if (dir_ni)
		ntfs_inode_close(dir_ni);
	free(path);
	return res;
}

static int ntfs_fuse_rm(const char *org_path)
{
	char *name;
	ntfschar *uname = NULL;
	ntfs_inode *dir_ni = NULL, *ni;
	char *path;
	int res = 0, uname_len;

	path = strdup(org_path);
	if (!path)
		return -errno;
	/* Open object for delete. */
	ni = ntfs_pathname_to_inode(ctx->vol, NULL, path);
	if (!ni) {
		res = -errno;
		goto exit;
	}
	/* Generate unicode filename. */
	name = strrchr(path, '/');
	name++;
	uname_len = ntfs_mbstoucs(name, &uname, 0);
	if (uname_len < 0) {
		res = -errno;
		goto exit;
	}
	/* Open parent directory. */
	*name = 0;
	dir_ni = ntfs_pathname_to_inode(ctx->vol, NULL, path);
	if (!dir_ni) {
		res = -errno;
		if (res == -ENOENT)
			res = -EIO;
		goto exit;
	}
	/* Delete object. */
	if (ntfs_delete(ni, dir_ni, uname, uname_len))
		res = -errno;
	ni = NULL;
exit:
	if (ni)
		ntfs_inode_close(ni);
	free(uname);
	if (dir_ni)
		ntfs_inode_close(dir_ni);
	free(path);
	return res;
}

static int ntfs_fuse_rm_stream(const char *path, ntfschar *stream_name,
		const int stream_name_len)
{
	ntfs_inode *ni;
	ntfs_attr *na;
	int res = 0;

	ni = ntfs_pathname_to_inode(ctx->vol, NULL, path);
	if (!ni)
		return -errno;
	na = ntfs_attr_open(ni, AT_DATA, stream_name, stream_name_len);
	if (!na) {
		res = -errno;
		goto exit;
	}
	if (ntfs_attr_rm(na)) {
		res = -errno;
		ntfs_attr_close(na);
	}
exit:
	if (ntfs_inode_close(ni))
		ntfs_log_perror("Failed to close inode");
	return res;
}

static int ntfs_fuse_unlink(const char *org_path)
{
	char *path = NULL;
	ntfschar *stream_name;
	int stream_name_len;
	int res = 0;

	stream_name_len = ntfs_fuse_parse_path(org_path, &path, &stream_name);
	if (stream_name_len < 0)
		return stream_name_len;
	if (!stream_name_len)
		res = ntfs_fuse_rm(path);
	else
		res = ntfs_fuse_rm_stream(path, stream_name, stream_name_len);
	ntfs_fuse_mark_free_space_outdated();
	free(path);
	if (stream_name_len)
		free(stream_name);
	return res;
}

static int ntfs_fuse_rename(const char *old_path, const char *new_path)
{
	int ret;

	if ((ret = ntfs_fuse_link(old_path, new_path)))
		return ret;
	if ((ret = ntfs_fuse_unlink(old_path))) {
		ntfs_fuse_unlink(new_path);
		return ret;
	}
	return 0;
}

static int ntfs_fuse_mkdir(const char *path,
		mode_t mode __attribute__((unused)))
{
	if (ntfs_fuse_is_named_data_stream(path))
		return -EINVAL; /* n/a for named data streams. */
	ntfs_fuse_mark_free_space_outdated();
	return ntfs_fuse_create(path, S_IFDIR, 0, NULL);
}

static int ntfs_fuse_rmdir(const char *path)
{
	if (ntfs_fuse_is_named_data_stream(path))
		return -EINVAL; /* n/a for named data streams. */
	ntfs_fuse_mark_free_space_outdated();
	return ntfs_fuse_rm(path);
}

static int ntfs_fuse_utime(const char *path, struct utimbuf *buf)
{
	ntfs_inode *ni;

	if (ntfs_fuse_is_named_data_stream(path))
		return -EINVAL; /* n/a for named data streams. */
	ni = ntfs_pathname_to_inode(ctx->vol, NULL, path);
	if (!ni)
		return -errno;
	if (buf) {
		ni->last_access_time = buf->actime;
		ni->last_data_change_time = buf->modtime;
		ni->last_mft_change_time = buf->modtime;
	} else {
		time_t now;

		now = time(NULL);
		ni->last_access_time = now;
		ni->last_data_change_time = now;
		ni->last_mft_change_time = now;
	}
	NInoFileNameSetDirty(ni);
	NInoSetDirty(ni);
	if (ntfs_inode_close(ni))
		ntfs_log_perror("Failed to close inode");
	return 0;
}

#ifdef HAVE_SETXATTR

static const char nf_ns_xattr_preffix[] = "user.";
static const int nf_ns_xattr_preffix_len = 5;

static int ntfs_fuse_listxattr(const char *path, char *list, size_t size)
{
	ntfs_attr_search_ctx *actx = NULL;
	ntfs_volume *vol;
	ntfs_inode *ni;
	char *to = list;
	int ret = 0;

	if (ctx->streams != NF_STREAMS_INTERFACE_XATTR)
		return -EOPNOTSUPP;
	vol = ctx->vol;
	if (!vol)
		return -ENODEV;
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni)
		return -errno;
	actx = ntfs_attr_get_search_ctx(ni, NULL);
	if (!actx) {
		ret = -errno;
		ntfs_inode_close(ni);
		goto exit;
	}
	while (!ntfs_attr_lookup(AT_DATA, NULL, 0, CASE_SENSITIVE,
				0, NULL, 0, actx)) {
		char *tmp_name = NULL;
		int tmp_name_len;

		if (!actx->attr->name_length)
			continue;
		tmp_name_len = ntfs_ucstombs((ntfschar *)((u8*)actx->attr +
				le16_to_cpu(actx->attr->name_offset)),
				actx->attr->name_length, &tmp_name, 0);
		if (tmp_name_len < 0) {
			ret = -errno;
			goto exit;
		}
		ret += tmp_name_len + nf_ns_xattr_preffix_len + 1;
		if (size) {
			if ((size_t)ret <= size) {
				strcpy(to, nf_ns_xattr_preffix);
				to += nf_ns_xattr_preffix_len;
				strncpy(to, tmp_name, tmp_name_len);
				to += tmp_name_len;
				*to = 0;
				to++;
			} else {
				free(tmp_name);
				ret = -ERANGE;
				goto exit;
			}
		}
		free(tmp_name);
	}
	if (errno != ENOENT)
		ret = -errno;
exit:
	if (actx)
		ntfs_attr_put_search_ctx(actx);
	ntfs_inode_close(ni);
	ntfs_log_debug("return %d\n", ret);
	return ret;
}

static int ntfs_fuse_getxattr_windows(const char *path, const char *name,
				char *value, size_t size)
{
	ntfs_attr_search_ctx *actx = NULL;
	ntfs_volume *vol;
	ntfs_inode *ni;
	char *to = value;
	int ret = 0;

	if (strcmp(name, "ntfs.streams.list"))
		return -EOPNOTSUPP;
	vol = ctx->vol;
	if (!vol)
		return -ENODEV;
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni)
		return -errno;
	actx = ntfs_attr_get_search_ctx(ni, NULL);
	if (!actx) {
		ret = -errno;
		ntfs_inode_close(ni);
		goto exit;
	}
	while (!ntfs_attr_lookup(AT_DATA, NULL, 0, CASE_SENSITIVE,
				0, NULL, 0, actx)) {
		char *tmp_name = NULL;
		int tmp_name_len;

		if (!actx->attr->name_length)
			continue;
		tmp_name_len = ntfs_ucstombs((ntfschar *)((u8*)actx->attr +
				le16_to_cpu(actx->attr->name_offset)),
				actx->attr->name_length, &tmp_name, 0);
		if (tmp_name_len < 0) {
			ret = -errno;
			goto exit;
		}
		if (ret)
			ret++; /* For space delimiter. */
		ret += tmp_name_len;
		if (size) {
			if ((size_t)ret <= size) {
				/* Don't add space to the beginning of line. */
				if (to != value) {
					*to = ' ';
					to++;
				}
				strncpy(to, tmp_name, tmp_name_len);
				to += tmp_name_len;
			} else {
				free(tmp_name);
				ret = -ERANGE;
				goto exit;
			}
		}
		free(tmp_name);
	}
	if (errno != ENOENT)
		ret = -errno;
exit:
	if (actx)
		ntfs_attr_put_search_ctx(actx);
	ntfs_inode_close(ni);
	return ret;
}

static int ntfs_fuse_getxattr(const char *path, const char *name,
				char *value, size_t size)
{
	ntfs_volume *vol;
	ntfs_inode *ni;
	ntfs_attr *na = NULL;
	ntfschar *lename = NULL;
	int res, lename_len;

	if (ctx->streams == NF_STREAMS_INTERFACE_WINDOWS)
		return ntfs_fuse_getxattr_windows(path, name, value, size);
	if (ctx->streams != NF_STREAMS_INTERFACE_XATTR)
		return -EOPNOTSUPP;
	if (strncmp(name, nf_ns_xattr_preffix, nf_ns_xattr_preffix_len) ||
			strlen(name) == (size_t)nf_ns_xattr_preffix_len)
		return -ENODATA;
	vol = ctx->vol;
	if (!vol)
		return -ENODEV;
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni)
		return -errno;
	lename_len = ntfs_mbstoucs(name + nf_ns_xattr_preffix_len, &lename, 0);
	if (lename_len == -1) {
		res = -errno;
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, lename, lename_len);
	if (!na) {
		res = -ENODATA;
		goto exit;
	}
	if (size) {
		if (size >= na->data_size) {
			res = ntfs_attr_pread(na, 0, na->data_size, value);
			if (res != na->data_size)
				res = -errno;
		} else
			res = -ERANGE;
	} else
		res = na->data_size;
exit:
	if (na)
		ntfs_attr_close(na);
	free(lename);
	if (ntfs_inode_close(ni))
		ntfs_log_perror("Failed to close inode");
	return res;
}

static int ntfs_fuse_setxattr(const char *path, const char *name,
				const char *value, size_t size, int flags)
{
	ntfs_volume *vol;
	ntfs_inode *ni;
	ntfs_attr *na = NULL;
	ntfschar *lename = NULL;
	int res, lename_len;

	if (ctx->streams != NF_STREAMS_INTERFACE_XATTR)
		return -EOPNOTSUPP;
	if (strncmp(name, nf_ns_xattr_preffix, nf_ns_xattr_preffix_len) ||
			strlen(name) == (size_t)nf_ns_xattr_preffix_len)
		return -EACCES;
	vol = ctx->vol;
	if (!vol)
		return -ENODEV;
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni)
		return -errno;
	lename_len = ntfs_mbstoucs(name + nf_ns_xattr_preffix_len, &lename, 0);
	if (lename_len == -1) {
		res = -errno;
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, lename, lename_len);
	if (na && flags == XATTR_CREATE) {
		res = -EEXIST;
		goto exit;
	}
	ntfs_fuse_mark_free_space_outdated();
	if (!na) {
		if (flags == XATTR_REPLACE) {
			res = -ENODATA;
			goto exit;
		}
		if (ntfs_attr_add(ni, AT_DATA, lename, lename_len, NULL, 0)) {
			res = -errno;
			goto exit;
		}
		na = ntfs_attr_open(ni, AT_DATA, lename, lename_len);
		if (!na) {
			res = -errno;
			goto exit;
		}
	}
	res = ntfs_attr_pwrite(na, 0, size, value);
	if (res != (s64) size)
		res = -errno;
	else
		res = 0;
exit:
	if (na)
		ntfs_attr_close(na);
	free(lename);
	if (ntfs_inode_close(ni))
		ntfs_log_perror("Failed to close inode");
	return res;
}

static int ntfs_fuse_removexattr(const char *path, const char *name)
{
	ntfs_volume *vol;
	ntfs_inode *ni;
	ntfs_attr *na = NULL;
	ntfschar *lename = NULL;
	int res = 0, lename_len;


	if (ctx->streams != NF_STREAMS_INTERFACE_XATTR)
		return -EOPNOTSUPP;
	if (strncmp(name, nf_ns_xattr_preffix, nf_ns_xattr_preffix_len) ||
			strlen(name) == (size_t)nf_ns_xattr_preffix_len)
		return -ENODATA;
	vol = ctx->vol;
	if (!vol)
		return -ENODEV;
	ni = ntfs_pathname_to_inode(vol, NULL, path);
	if (!ni)
		return -errno;
	lename_len = ntfs_mbstoucs(name + nf_ns_xattr_preffix_len, &lename, 0);
	if (lename_len == -1) {
		res = -errno;
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, lename, lename_len);
	if (!na) {
		res = -ENODATA;
		goto exit;
	}
	ntfs_fuse_mark_free_space_outdated();
	if (ntfs_attr_rm(na))
		res = -errno;
	else
		na = NULL;
exit:
	if (na)
		ntfs_attr_close(na);
	free(lename);
	if (ntfs_inode_close(ni))
		ntfs_log_perror("Failed to close inode");
	return res;
}

#endif /* HAVE_SETXATTR */

static struct fuse_operations ntfs_fuse_oper = {
	.getattr	= ntfs_fuse_getattr,
	.readlink	= ntfs_fuse_readlink,
	.readdir	= ntfs_fuse_readdir,
	.open		= ntfs_fuse_open,
	.read		= ntfs_fuse_read,
	.write		= ntfs_fuse_write,
	.truncate	= ntfs_fuse_truncate,
	.statfs		= ntfs_fuse_statfs,
	.chmod		= ntfs_fuse_chmod,
	.chown		= ntfs_fuse_chown,
	.mknod		= ntfs_fuse_mknod,
	.symlink	= ntfs_fuse_symlink,
	.link		= ntfs_fuse_link,
	.unlink		= ntfs_fuse_unlink,
	.rename		= ntfs_fuse_rename,
	.mkdir		= ntfs_fuse_mkdir,
	.rmdir		= ntfs_fuse_rmdir,
	.utime		= ntfs_fuse_utime,
#ifdef HAVE_SETXATTR
	.getxattr	= ntfs_fuse_getxattr,
	.setxattr	= ntfs_fuse_setxattr,
	.removexattr	= ntfs_fuse_removexattr,
	.listxattr	= ntfs_fuse_listxattr,
#endif /* HAVE_SETXATTR */
};

static int ntfs_fuse_init(void)
{
	ctx = malloc(sizeof(ntfs_fuse_context_t));
	if (!ctx) {
		ntfs_log_perror("malloc failed");
		return -1;
	}
	*ctx = (ntfs_fuse_context_t) {
		.state = NF_FreeClustersOutdate | NF_FreeMFTOutdate,
		.uid = geteuid(),
		.gid = getegid(),
		.fmask = 0177,
		.dmask = 0077,
		.streams = NF_STREAMS_INTERFACE_NONE,
	};
	return 0;
}

static int ntfs_fuse_mount(const char *device)
{
	ntfs_volume *vol;

	vol = utils_mount_volume(device, ((ctx->ro) ? MS_RDONLY : 0) |
			((ctx->noatime) ? MS_NOATIME : 0), ctx->force);
	if (!vol) {
		ntfs_log_error("Mount failed.\n");
		return -1;
	}
	ctx->vol = vol;
	return 0;
}

static void ntfs_fuse_destroy(void)
{
	if (ctx->vol) {
		ntfs_log_info("Unmounting %s (%s)\n", opts.device,
				ctx->vol->vol_name);
		if (ntfs_umount(ctx->vol, FALSE))
			ntfs_log_perror("Failed to unmount volume");
	}
	free(ctx);
	free(opts.device);
}

static void signal_handler(int arg __attribute__((unused)))
{
	fuse_exit((fuse_get_context())->fuse);
}

static char *parse_mount_options(const char *org_options)
{
	char *options, *s, *opt, *val, *ret;
	BOOL no_def_opts = FALSE;

	/*
	 * +7		for "fsname=".
	 * +1		for comma.
	 * +1		for null-terminator.
	 * +PATH_MAX	for resolved by realpath() device name.
	 */
	ret = malloc(strlen(def_opts) + strlen(org_options) + 9 + PATH_MAX);
	if (!ret) {
		ntfs_log_perror("malloc failed");
		return NULL;
	}
	*ret = 0;
	options = strdup(org_options);
	if (!options) {
		ntfs_log_perror("strdup failed");
		return NULL;
	}
	s = options;
	while ((val = strsep(&s, ","))) {
		opt = strsep(&val, "=");
		if (!strcmp(opt, "ro")) { /* Read-only mount. */
			if (val) {
				ntfs_log_error("'ro' option should not have "
						"value.\n");
				goto err_exit;
			}
			ctx->ro = TRUE;
			strcat(ret, "ro,");
		} else if (!strcmp(opt, "noatime")) {
			if (val) {
				ntfs_log_error("'noatime' option should not "
						"have value.\n");
				goto err_exit;
			}
			ctx->noatime = TRUE;
			strcat(ret, "noatime,"); /* Duplicate it for FUSE. */
		} else if (!strcmp(opt, "fake_rw")) {
			if (val) {
				ntfs_log_error("'fake_rw' option should not "
						"have value.\n");
				goto err_exit;
			}
			ctx->ro = TRUE;
		} else if (!strcmp(opt, "fsname")) { /* Filesystem name. */
			/*
			 * We need this to be able to check whether filesystem
			 * mounted or not.
			 */
			ntfs_log_error("'fsname' is unsupported option.\n");
			goto err_exit;
		} else if (!strcmp(opt, "no_def_opts")) {
			if (val) {
				ntfs_log_error("'no_def_opts' option should "
						"not have value.\n");
				goto err_exit;
			}
			no_def_opts = TRUE; /* Don't add default options. */
		} else if (!strcmp(opt, "umask")) {
			if (!val) {
				ntfs_log_error("'umask' option should have "
						"value.\n");
				goto err_exit;
			}
			sscanf(val, "%i", &ctx->fmask);
			ctx->dmask = ctx->fmask;
		} else if (!strcmp(opt, "fmask")) {
			if (!val) {
				ntfs_log_error("'fmask' option should have "
						"value.\n");
				goto err_exit;
			}
			sscanf(val, "%i", &ctx->fmask);
		} else if (!strcmp(opt, "dmask")) {
			if (!val) {
				ntfs_log_error("'dmask' option should have "
						"value.\n");
				goto err_exit;
			}
			sscanf(val, "%i", &ctx->dmask);
		} else if (!strcmp(opt, "uid")) {
			if (!val) {
				ntfs_log_error("'uid' option should have "
						"value.\n");
				goto err_exit;
			}
			sscanf(val, "%i", &ctx->uid);
		} else if (!strcmp(opt, "gid")) {
			if (!val) {
				ntfs_log_error("'gid' option should have "
						"value.\n");
				goto err_exit;
			}
			sscanf(val, "%i", &ctx->gid);
		} else if (!strcmp(opt, "show_sys_files")) {
			if (val) {
				ntfs_log_error("'show_sys_files' option should "
						"not have value.\n");
				goto err_exit;
			}
			ctx->show_sys_files = TRUE;
		} else if (!strcmp(opt, "silent")) {
			if (val) {
				ntfs_log_error("'silent' option should "
						"not have value.\n");
				goto err_exit;
			}
			ctx->silent = TRUE;
		} else if (!strcmp(opt, "force")) {
			if (val) {
				ntfs_log_error("'force' option should not "
						"have value.\n");
				goto err_exit;
			}
			ctx->force = TRUE;
		} else if (!strcmp(opt, "locale")) {
			if (!val) {
				ntfs_log_error("'locale' option should have "
						"value.\n");
				goto err_exit;
			}
			if (!setlocale(LC_ALL, val))
				ntfs_log_error("Failed to set locale to %s.  "
						"Continue anyway.\n", val);
		} else if (!strcmp(opt, "streams_interface")) {
			if (!val) {
				ntfs_log_error("'streams_interface' option "
						"should have value.\n");
				goto err_exit;
			}
			if (!strcmp(val, "none"))
				ctx->streams = NF_STREAMS_INTERFACE_NONE;
			else if (!strcmp(val, "xattr"))
				ctx->streams = NF_STREAMS_INTERFACE_XATTR;
			else if (!strcmp(val, "windows"))
				ctx->streams = NF_STREAMS_INTERFACE_WINDOWS;
			else {
				ntfs_log_error("Invalid named data streams "
						"access interface.\n");
				goto err_exit;
			}
		} else if (!strcmp(opt, "noauto")) {
			/* Don't pass noauto option to fuse. */
		} else if (!strcmp(opt, "debug")) {
			if (val) {
				ntfs_log_error("'debug' option should not have "
						"value.\n");
				goto err_exit;
			}
			ctx->debug = TRUE;
			ntfs_log_set_levels(NTFS_LOG_LEVEL_DEBUG);
			ntfs_log_set_levels(NTFS_LOG_LEVEL_TRACE);
		} else if (!strcmp(opt, "no_detach")) {
			if (val) {
				ntfs_log_error("'no_detach' option should not "
						"have value.\n");
				goto err_exit;
			}
			ctx->no_detach = TRUE;
		} else if (!strcmp(opt, "remount")) {
			ntfs_log_error("Remounting is not supported at present."
					" You have to umount volume and then "
					"mount it once again.\n");
			goto err_exit;
		} else { /* Probably FUSE option. */
			strcat(ret, opt);
			if (val) {
				strcat(ret, "=");
				strcat(ret, val);
			}
			strcat(ret, ",");
		}
	}
	if (!no_def_opts)
		strcat(ret, def_opts);
	strcat(ret, "fsname=");
	strcat(ret, opts.device);
exit:
	free(options);
	return ret;
err_exit:
	free(ret);
	ret = NULL;
	goto exit;
}

static void usage(void)
{
	ntfs_log_info("\n%s v%s (libntfs %s) - NTFS module for FUSE.\n\n",
			EXEC_NAME, VERSION, ntfs_libntfs_version());
	ntfs_log_info("Copyright (C) 2005-2006 Yura Pakhuchiy\n\n");
	ntfs_log_info("usage:  %s device mount_point [-o options]\n\n",
			EXEC_NAME);
	ntfs_log_info("ntfsmount options are:\n\tforce\n\tno_def_opts\n\tumask"
		"\n\tfmask\n\tdmask\n\tuid\n\tgid\n\tshow_sys_files\n\t"
		"silent\n\tlocale\n\tstreams_interface\n"
		"Also look into FUSE documentation about it options "
		"(NOTE: not all FUSE options are supported by ntfsmount).\n");
	ntfs_log_info("Default options are: \"%s\".\n\n", def_opts);
	ntfs_log_info("%s%s\n", ntfs_bugs, ntfs_home);
}

#ifndef HAVE_REALPATH
/* If there is no realpath() on the system, provide a dummy one. */
static char *realpath(const char *path, char *resolved_path)
{
	strncpy(resolved_path, path, PATH_MAX);
	resolved_path[PATH_MAX] = '\0';
	return resolved_path;
}
#endif

/**
 * parse_options - Read and validate the programs command line
 *
 * Read the command line, verify the syntax and parse the options.
 * This function is very long, but quite simple.
 *
 * Return:  1 Success
 *	    0 Error, one or more problems
 */
static int parse_options(int argc, char *argv[])
{
	int err = 0, help = 0;
	int c = -1;

	static const char *sopt = "-o:h?qv";
	static const struct option lopt[] = {
		{ "options",	 required_argument,	NULL, 'o' },
		{ "help",	 no_argument,		NULL, 'h' },
		{ "quiet",	 no_argument,		NULL, 'q' },
		{ "verbose",	 no_argument,		NULL, 'v' },
		{ NULL,		 0,			NULL,  0  }
	};

	opterr = 0; /* We'll handle the errors, thank you. */

	opts.mnt_point = NULL;
	opts.options = NULL;
	opts.device = NULL;

	while ((c = getopt_long(argc, argv, sopt, lopt, NULL)) != -1) {
		switch (c) {
		case 1:	/* A non-option argument */
			if (!opts.device) {
				opts.device = malloc(PATH_MAX + 1);
				if (!opts.device) {
					ntfs_log_perror("malloc");
					err++;
					break;
				}
				/* We don't want relative path in /etc/mtab. */
				if (argv[optind - 1][0] != '/') {
					if (!realpath(argv[optind - 1],
							opts.device)) {
						ntfs_log_perror("realpath");
						free(opts.device);
						opts.device = NULL;
						err++;
						break;
					}
				} else
					strcpy(opts.device, argv[optind - 1]);
			} else if (!opts.mnt_point)
				opts.mnt_point = argv[optind - 1];
			else {
				ntfs_log_error("You must specify exactly one "
						"device and exactly one mount "
						"point.\n");
				err++;
			}
			break;
		case 'o':
			if (!opts.options)
				opts.options = argv[optind - 1];
			else {
				ntfs_log_error("You must specify exactly one "
						"set of options.\n");
				err++;
			}
			break;
		case 'h':
		case '?':
			help++;
			break;
		case 'q':
			opts.quiet++;
			break;
		case 'v':
			opts.verbose++;
			break;
		default:
			ntfs_log_error("Unknown option '%s'.\n",
					argv[optind - 1]);
			err++;
			break;
		}
	}

	if (help) {
		opts.quiet = 0;
	} else {
		if (!opts.device) {
			ntfs_log_error("No device specified.\n");
			err++;
		}

		if (opts.quiet && opts.verbose) {
			ntfs_log_error("You may not use --quiet and --verbose "
					"at the same time.\n");
			err++;
		}
	}

	if (help || err)
		usage();

	return (!help && !err);
}

int main(int argc, char *argv[])
{
	char *parsed_options;
#if defined(FUSE_VERSION) && (FUSE_VERSION >= 25)
	struct fuse_args margs = FUSE_ARGS_INIT(0, NULL);
#endif
	struct fuse *fh;
	int ffd = 0;

	utils_set_locale();
	ntfs_log_set_handler(ntfs_log_handler_stderr);
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	if (!parse_options(argc, argv))
		return 1;

	ntfs_fuse_init();
	/* Parse options. */
	parsed_options = parse_mount_options((opts.options) ?
			opts.options : "");
	if (!parsed_options) {
		ntfs_fuse_destroy();
		return 3;
	}

	/* Mount volume. */
	if (ntfs_fuse_mount(opts.device)) {
		ntfs_fuse_destroy();
		return 4;
	}
	/* Create filesystem. */
#if defined(FUSE_VERSION) && (FUSE_VERSION >= 25)
	if ((fuse_opt_add_arg(&margs, "") == -1 ||
			fuse_opt_add_arg(&margs, "-o") == -1 ||
			fuse_opt_add_arg(&margs, parsed_options) == -1))
		ffd = -1;
	if (ffd != -1)
		ffd = fuse_mount(opts.mnt_point, &margs);
	fuse_opt_free_args(&margs);
#else
	ffd = fuse_mount(opts.mnt_point, parsed_options);
#endif
	if (ffd == -1) {
		ntfs_log_error("fuse_mount failed.\n");
		ntfs_fuse_destroy();
		return 5;
	}
	free(parsed_options);
#if defined(FUSE_VERSION) && (FUSE_VERSION >= 25)
	fh = (struct fuse *)1; /* Cast anything except NULL to handle errors. */
	margs = (struct fuse_args)FUSE_ARGS_INIT(0, NULL);
	if (fuse_opt_add_arg(&margs, "") == -1 ||
			fuse_opt_add_arg(&margs, "-o") == -1)
		    fh = NULL;
	if (!ctx->debug && !ctx->no_detach) {
		if (fuse_opt_add_arg(&margs, "use_ino,kernel_cache") == -1)
			fh = NULL;
	} else {
		if (fuse_opt_add_arg(&margs, "use_ino,debug") == -1)
			fh = NULL;
	}
	if (fh)
		fh = fuse_new(ffd, &margs , &ntfs_fuse_oper,
				sizeof(ntfs_fuse_oper));
	fuse_opt_free_args(&margs);
#else
	if (!ctx->debug && !ctx->no_detach) {
		if (fuse_is_lib_option("kernel_cache"))
			fh = fuse_new(ffd, "use_ino,kernel_cache",
					&ntfs_fuse_oper,
					sizeof(ntfs_fuse_oper));
		else
			fh = fuse_new(ffd, "use_ino", &ntfs_fuse_oper,
					sizeof(ntfs_fuse_oper));
	} else
		fh = fuse_new(ffd, "debug,use_ino" , &ntfs_fuse_oper,
				sizeof(ntfs_fuse_oper));
#endif
	if (!fh) {
		ntfs_log_error("fuse_new failed.\n");
		close(ffd);
		fuse_unmount(opts.mnt_point);
		ntfs_fuse_destroy();
		return 6;
	}
	if (!ctx->debug && !ctx->no_detach) {
		if (daemon(0, 0))
			ntfs_log_error("Failed to daemonize.\n");
		else {
			ntfs_log_set_handler(ntfs_log_handler_syslog);
			/* Override default libntfs identify. */
			openlog(EXEC_NAME, LOG_PID, LOG_DAEMON);
		}
	}
	ntfs_log_info("Version %s (libntfs %s)\n", VERSION,
			ntfs_libntfs_version());
	ntfs_log_info("Mounted %s (%s, label \"%s\", volume version %d.%d)\n",
			opts.device, (ctx->ro) ? "RO" : "RW",
			ctx->vol->vol_name, ctx->vol->major_ver,
			ctx->vol->minor_ver);
	/* Main loop. */
	fuse_loop(fh);
	/* Destroy. */
	fuse_destroy(fh);
	close(ffd);
	fuse_unmount(opts.mnt_point);
	ntfs_fuse_destroy();
	return 0;
}
