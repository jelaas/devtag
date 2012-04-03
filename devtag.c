/*
 * File: devtag.c
 * Implements: displaying deviceinformation in format suitable for devname cmd
 * Implements: device name lookup via command
 * Implements: device name configuration
 * Implements: executes command in a mount namespace with a fixed named devicenode
 *
 * Copyright: Jens Låås, 2011
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#define _GNU_SOURCE
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <syscall.h>
#define unshare(flags) ((long)syscall(SYS_unshare, flags))

#include "libdevtag.h"
#include "jelopt.h"
#include "devtag.h"

struct {
	int list, ns, create;
} conf;

static int strsuffix(const char *s, const char *suf)
{
	if(strlen(suf) > strlen(s))
		return 0;
	s+=(strlen(s)-strlen(suf));
	return strcmp(s, suf)==0;
}

static void conf_lookup(const char *pfx, DIR *dir)
{
	struct dirent *ent;
	char *p, name[256], node[256];
	while( (ent=readdir(dir)) ) {
		if(strsuffix(ent->d_name, ".conf")) {
			strncpy(name, ent->d_name, sizeof(name));
			p = strrchr(name, '.');
			if(p) *p = 0;
			if(devtag_lookup(node, sizeof(node), name)>0)
				printf("%s: %s -> %s\n", pfx, name, node);
			else
				printf("%s: %s not found\n", pfx, name);
		}
	}	
}

int main(int argc, char **argv)
{
	int rc=0, err=0;
	struct dev_head result;
	struct devinfo_head sel;
	struct dev *dev;
	struct devinfo *info;
	struct devname *d;
	char buf[512];

	result.head = NULL;
	sel.head = NULL;

	if(strcmp(basename(argv[0]), "devtagns")==0)
		conf.ns = 1;
	
	if(jelopt(argv, 'h', "help", NULL, &err)) {
	usage:
		printf("devtag [-hl] [--ns TAG CMD..] [-c TAG SEL..] [TAG]\n"
		       " version " LIBDEVTAG_VERSION "\n"
		       " -l --list\n"
		       "      Perform lookup of all tags created (system and user).\n"
		       "      Tags specified system wide (in /etc/devtag.d) are prefixed with \"sys:\".\n" 
		       "      Per user tags (in $HOME/.devtag.d) are prefixed with \"user:\".\n" 
		       " --ns TAG CMD ARG*\n"
		       "      Runs 'CMD' or bash in a separate mount name space.\n"
		       "      This name space has its own mount of /dev with a devicenode created\n"
		       "      according to configuration of the devtag 'TAG'.\n"
		       " -c --create TAG SELECTOR+\n"
		       "      'TAG' is your handle for the device that matches the given selectors.\n"
		       "      'SELECTOR' is a 'name=value' pair. Suitable selectors is reported by invocing\n"
		       "      devtag without any arguments.\n"
		       " Without an argument:\n"
		       "      Scans for and prints device information suitable as selectors for the\n"
		       "      --create option.\n"
		       " Without exactly one argument [TAG]:\n"
		       "      Lookup device node for devtag 'TAG'. Print result on stdout.\n"
		       " If invoked as 'devtagns' the program will behave as the '--ns' option was given.\n"
		       "\n"
		       " Magic selectors (maybe not reported by devtag invocation):\n"
		       "  dev=<pattern>               device name pattern for selecting device node\n"
		       "  devname=<nodename>          devname-set will use this constant name for the device node\n"
		       "  class=<subsystem>           class is usb by default\n"
		       "\n"
		       "The devtag configurations are saved in '/etc/devtag.d' or '$HOME/.devtag.d'.\n"
			);
		exit(0);
	}
			
	if(!conf.ns) {
		while(jelopt(argv, 0, "ns", NULL, &err)) {
			conf.ns = 1;
		}
		while(jelopt(argv, 'c', "create", NULL, &err)) {
			conf.create = 1;
		}
		if(jelopt(argv, 'l', "list", NULL, &err)) {
			DIR *dir;
			char *home, fn[256];
			
			strcpy(fn, "/etc/devtag.d");
			dir = opendir(fn);
			if(dir) {
				conf_lookup("sys", dir);
				closedir(dir);
			}
			home = getenv("HOME");
			if(home) {
				snprintf(fn, sizeof(fn), "%s/.devtag.d", home);
				dir = opendir(fn);
				if(dir) {
					conf_lookup("user", dir);
					closedir(dir);
				}
			}
			exit(0);
		}

	}

	argc = jelopt_final(argv, &err);

	/* run command in a separate namespace */
	if(conf.ns) {
		char fn[256];
		char dev[512];
		char devname[512];
		mode_t mask;
		struct stat node;
		
		if(argc <= 1) {
			rc=1;
			goto usage;
		}
		
		if(devtag_lookup2(dev, sizeof(dev), devname, sizeof(devname), argv[1]) <= 0) {
			fprintf(stderr, "%s not found\n", argv[1]);
			return 1;
		}
		
		if(!devname[0]) {
			strcpy(devname, argv[1]);
		}
		
		/* save info about dev/<dev> */
		sprintf(fn, "/dev/%s", dev);
		if(stat(fn, &node)) {
			fprintf(stderr, "Could not stat device node %s\n", fn);
			exit(1);
		}
		
		/* unshare mount namespace */
		if(unshare(CLONE_NEWNS)) {
			fprintf(stderr, "failed to unshare mount namespace\n");
			exit(1);
		}
		
		/* mount tmpfs on /dev */
		if(mount("tmpfs", "/dev", "tmpfs", 0, NULL)) {
			fprintf(stderr, "failed to mount tmpfs on /dev\n");
			exit(1);
		}
		
		/* create devicenode /dev/<devname> as a copy of /dev/<dev> */
		sprintf(fn, "/dev/%s", devname);
		mask = umask(0);
		if(mknod(fn, node.st_mode, node.st_rdev)) {
			fprintf(stderr, "Could not create device node %s\n", fn);
			exit(1);
		}
		umask(mask);
		
		chown(fn, node.st_uid, node.st_gid);
		
		/* exec argv[2] */
		
		argc--;
		argv++;
		argc--;
		argv++;
		if (!argc) {
			execl("/bin/sh", "-sh", NULL);
			exit(2);
		}
		
		execvp(argv[0], argv);
		exit(3);
	}

	if(conf.create) {
		int fd, i, err=0;
		char *dev;
		mode_t mask;
		char fn[256];
		
		argc = jelopt_final(argv, &err);
		
		if(argc < 3) goto usage;
		
		dev=argv[1];
		if(getuid()==0) {
			mask = umask(0);
			sprintf(fn, "/etc/devtag.d/%s.conf", dev);
			mkdir("/etc/devtag.d", 0755);
			fd = open(fn, O_RDWR|O_CREAT|O_EXCL, 0644);
			umask(mask);
		} else {
			sprintf(fn, "%s/.devtag.d", getenv("HOME"));
			mkdir(fn, 0755);
			sprintf(fn, "%s/.devtag.d/%s.conf", getenv("HOME"), dev);
			fd = open(fn, O_RDWR|O_CREAT|O_EXCL, 0644);
		}
		if(fd == -1) {
			printf("Cannot create %s (must not already exist!)\n", fn);
			exit(1);
		}
		argc--;
		argv++;
		
		/* build selector list */
		for(i=1;i<argc;i++) {
			if(strchr(argv[i], '=')) {
				write(fd, argv[i], strlen(argv[i]));
				write(fd, "\n", 1);
			}
		}
		close(fd);
		
		return 0;
	}
	
	/* No argument given. Display information of available selectors. */
	if(argc < 2) {
		devtag_usb_scan(&result, &sel);
		
		for(dev=result.head;dev;dev=dev->next) {
			int controller=0;
			
			/* filter out the system USB controllers */
			for(info=dev->info.head;info;info=info->next) {
				if(strcmp(info->name, "idVendor")==0) {
					if(strcmp(info->value, "1d6b")==0)
						controller++;
				}
				if(strcmp(info->name, "idProduct")==0) {
					if(strncmp(info->value, "000", 3)==0)
						controller++;
				}
			}
			if(controller==2) continue;
			
			for(info=dev->info.head;info;info=info->next) {
				printf("%s=\"%s\" ", info->name, info->value);
			}
			for(d=dev->devnames.head;d;d=d->next) {
				char fn[256];
				struct stat statb;
				sprintf(fn, "/dev/%s", d->devname);
				if(stat(fn, &statb)==0) {
					printf("dev=%s ", d->devname);
					continue;
				}
				sprintf(fn, "/dev/input/%s", d->devname);
				if(stat(fn, &statb)==0) {
					printf("dev=%s ", d->devname);
					continue;
				}
			}
			printf("\n");
		}
		exit(0);
	}
	
	if(argc <= 1) {
		rc=1;
		goto usage;
	}

	if(devtag_lookup(buf, sizeof(buf), argv[1])>0)
		printf("%s\n", buf);
	else {
		fprintf(stderr, "%s not found\n", argv[1]);
		exit(1);
	}
	exit(0);
}
