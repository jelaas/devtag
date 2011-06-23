/*
 * File: lookup.c
 * Implements: device name lookup functions
 *
 * Copyright: Jens Låås, 2011
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fnmatch.h>
#include <stdlib.h>

#include "devtag.h"
#include "libdevtag.h"

static int parse(const char *devname, char **class, char **devpattern, char **constdev, struct devinfo_head *sel)
{
	char *home, *pp, *p, *name, *value, fn[256], buf[512];
	int fd;
	ssize_t n;
	struct devinfo *di;
	
	*class = NULL;
	*devpattern = NULL;
	*constdev = NULL;
	
	sprintf(fn, "/etc/devtag.d/%s.conf", devname);
	fd = open(fn, O_RDONLY);
	if(fd == -1) {
		home = getenv("HOME");
		if(home) {
			sprintf(fn, "%s/.devtag.d/%s.conf", home, devname);
			fd = open(fn, O_RDONLY);
		}
	}
	if(fd == -1) return -1;
	
	n = read(fd, buf, sizeof(buf)-1);
	if(n <= 0)
		return -2;
	
	buf[n] = 0;

	p = buf;
	while(p && *p) {
		name = p;
		p = strchr(p, '\n');
		if(p) {
			*p = 0;
			p++;
		}
		value = strchr(name, '=');
		if(value) {
			*value = 0;
			value++;
			if(*value == '"') {
				value++;
				pp = strchr(value, '"');
				if(pp) *pp = 0;
			}
			
			if(!strcmp(name, "class")) {
				*class = strdup(value);
				continue;
			}
			if(!strcmp(name, "dev")) {
				*devpattern = strdup(value);
				continue;
			}
			if(!strcmp(name, "devname")) {
				*constdev = strdup(value);
				continue;
			}
			
			di = malloc(sizeof(struct devinfo));
			if(di) {
				di->name = strdup(name);
				di->value = strdup(value);
				di->next = sel->head;
				sel->head = di;
			}
		}
	}

	close(fd);
	return 0;
}

static char *dev_match(struct dev *dev, char *devpattern)
{
	struct devname *devname;
	
	for(devname=dev->devnames.head;devname;devname=devname->next) {
		if(fnmatch(devpattern, devname->devname, 0)==0) {
			return devname->devname;
		}
	}
	return NULL;
}

int devtag_lookup2(char *buf, size_t bufsize, char *constbuf, size_t constsize, const char *devname)
{
	struct devinfo_head sel;
	struct dev_head result;
	struct dev *dev;
	char *class, *devpattern, *constdev, *pfx="";
	struct devname *dn;
	int len = 0;
	
	sel.head = NULL;
	result.head = NULL;
	
	snprintf(buf, bufsize, "%s", devname);
	
	if(strncmp(devname, "/dev/", 5)==0) {
		pfx = "/dev/";
		devname += 5;
	}

	/* parse config file in /etc/devtag.d/ */
	if(parse(devname, &class, &devpattern, &constdev, &sel))
		return -1;
	if(constbuf) {
		constbuf[0] = 0;
		if(constdev)
			strncpy(constbuf, constdev, constsize-1);
		constbuf[constsize-1] = 0;
	}
	
	if(!class) class = "usb";

	/* look for usb devices */
	if(strcmp(class, "usb")==0)
		devtag_usb_scan(&result, &sel);

	for(dev=result.head;dev;dev=dev->next) {
		len++;
	}

	if(!devpattern) {
		dev = result.head;
		if(dev) {
			dn = dev->devnames.head;
			if(dn) {
				snprintf(buf, bufsize, "%s%s", pfx, dn->devname);
				return len;
			}
		}
	}

	/* check for matching device node with devpattern among the devices that matched
	   selectors */
	for(dev=result.head;dev;dev=dev->next) {
		/* find any matching devicenode */
		if((devname = dev_match(dev, devpattern))) {
			snprintf(buf, bufsize, "%s%s", pfx, devname);
			return len;
		}
	}
	return 0;
}

int devtag_lookup(char *buf, size_t bufsize, const char *devname)
{
	return devtag_lookup2(buf, bufsize, NULL, 0, devname);
}

char *devname_get(const char *devname)
{
	char buf[64];
	buf[0] = 0;
	devtag_lookup(buf, sizeof(buf), devname);
	return strdup(buf);
}
