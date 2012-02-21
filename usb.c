/*
 * File: usb.c
 * Implements: scanning of usb devices
 *
 * Copyright: Jens Låås, 2011
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <ctype.h>
#include <fnmatch.h>

#ifdef DEVTAG_ALLINONE
#include "devtag-allinone.h"
#else
#include "libdevtag.h"
#endif

static int usb_scan_dir(struct dev_head *result, const struct devinfo_head *sel, const char *dir);

static char *getstring(const char *dir, const char *file)
{
	char fn[512];
	char buf[256];
	int fd, n;
	
	sprintf(fn, "%s/%s", dir, file);
	fd = open(fn, O_RDONLY);
	if(fd == -1) return NULL;

	n = read(fd, buf, sizeof(buf)-1);
	if(n >= 0) {
		buf[n] = 0;
		if(n > 0) {
			if(buf[n-1] == '\n')
				buf[n-1] = 0;
		}
	} else buf[0] = 0;
	buf[sizeof(buf)-1] = 0;
	close(fd);
	return strdup(buf);
}

/*
 * scan usb devices.
 * add matching devices to result list
 */
int devtag_usb_scan(struct dev_head *result, const struct devinfo_head *sel)
{
	return usb_scan_dir(result, sel, "/sys/bus/usb/devices");
}

static char *dev_probe(const char *name)
{
	char fn[512];
	struct stat statb;

	sprintf(fn, "/dev/%s", name);
	if(stat(fn, &statb)==0) {
		if(S_ISBLK(statb.st_mode))
			return "b";
		if(S_ISCHR(statb.st_mode))
			return "c";
	}
	return "f";
}

static struct devname *devname_new(const char *dir)
{
	struct devname *dn;
	char *name;

	name = basename(strdup(dir));
	if(isdigit(name[0]) )
		return NULL;
	
	dn = malloc(sizeof(struct devname));
	dn->dev = getstring(dir, "dev");
	dn->devname = name;
	dn->type = dev_probe(name);
	dn->pos = strlen(dir);
	dn->next = NULL;
//	printf("devname_new %s %s %s\n", dn->dev, dn->devname, dir);
	return dn;
}

/*
 * depth-first scan for all dev files. name of parentdir is assumed to be node name.
 */
static int usb_scan_devname(const char *dir, struct devname_head *devnames)
{
	DIR *d;
	struct dirent *ent;
	char fn[512];
	int len;
	
//	printf("scan_devname opening dir %s\n", dir);
	d = opendir(dir);
	if(!d) return 0;
	
	while((ent = readdir(d))) {
		if(ent->d_type == DT_LNK) continue;
		if(ent->d_type == DT_UNKNOWN) {
			/* FIXME: use lstat to determine if file is a link */
			printf("DT_UNKNOWN\n");
			exit(1);
		}
		if(ent->d_name[0] != '.') {
			sprintf(fn, "%s/%s", dir, ent->d_name);
			usb_scan_devname(fn, devnames);
		}
		if(!strcmp(ent->d_name, "dev")) {
			struct devname *devname;
			devname = devname_new(dir);
			if(devname) {
				devname->next = devnames->head;
				devnames->head = devname;
			}
		}
	}
	closedir(d);
	
	{
		struct devname *dn;
		len=0;
		for(dn=devnames->head;dn;dn=dn->next)
			len++;
	}
	return len;
}

static int dev_info_add(struct dev *dev, const char *dir, const char *name)
{
	struct devinfo *info;
	char *value;
	
	value = getstring(dir, name);
	
	if(value) {
		info = malloc(sizeof(struct devinfo));
		info->name = name;
		info->value = value;
	
		info->next = dev->info.head;
		dev->info.head = info;
		return 0;
	}
	return -1;
}

static struct dev *dev_new()
{
	struct dev *d;
	d = malloc(sizeof(struct dev));
	d->class = "usb";
	d->next = NULL;
	return d;
}

static int usb_dev_match(const struct devinfo_head *info, const struct devinfo_head *selectors)
{
	int match=1;
	struct devinfo *sel, *i;
	
	for(sel=selectors->head;sel;sel=sel->next) {
		match=0;
		for(i=info->head;i;i=i->next) {
			if(strcmp(i->name, sel->name)==0) {
				if(fnmatch(sel->value, i->value, 0)==0) {
					match=1;
					break;
				}
			}
		}
		if(match==0)
			return 0;
	}
	return match;
}

static int usb_scan_dev(struct dev_head *result, const struct devinfo_head *sel, const char *dir)
{
	char *usbdev;
	struct devname_head devnames;
	struct dev *dev;
	
	usbdev = getstring(dir, "dev");
	if(usbdev) {
		devnames.head = NULL;
		
		if(usb_scan_devname(dir, &devnames)) {
			dev = dev_new();
			dev->devnames.head = devnames.head;
			dev->info.head = NULL;
			dev_info_add(dev, dir, "serial");
			dev_info_add(dev, dir, "manufacturer");
			dev_info_add(dev, dir, "product");
			dev_info_add(dev, dir, "idProduct");
			dev_info_add(dev, dir, "idVendor");

			if(usb_dev_match(&dev->info, sel)) {
				dev->next = result->head;
				result->head = dev;
			}
		}
	}
	return 0;
}

static int usb_scan_dir(struct dev_head *result, const struct devinfo_head *sel, const char *dir)
{
	DIR *d;
	struct dirent *ent;
	char fn[512];
	
/*
 /sys/bus/usb/devices/usb1/1-4/serial
 /sys/bus/usb/devices/1-4:1.0/serial

 */
//	printf("opening dir %s\n", dir);
	d = opendir(dir);
	if(!d) return 0;

	while((ent = readdir(d))) {
		if(ent->d_name[0] != '.') {
			sprintf(fn, "%s/%s", dir, ent->d_name);
			usb_scan_dev(result, sel, fn);
		}
	}
	closedir(d);
	return 0;
}
