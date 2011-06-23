/*
 * File: dev.c
 * Implements: main entry point for devicename scanning
 *
 * Copyright: Jens Låås, 2011
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include "libdevtag.h"

int devtag_dev_scan(struct dev_head *result, const struct devinfo_head *sel)
{
	return devtag_usb_scan(result,sel);
}
