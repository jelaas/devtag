/*
 * File: dev.c
 * Implements: main entry point for devicename scanning
 *
 * Copyright: Jens L��s, 2011
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#ifdef DEVTAG_ALLINONE
#include "devtag-allinone.h"
#else
#include "libdevtag.h"
#endif

int devtag_dev_scan(struct dev_head *result, const struct devinfo_head *sel)
{
	return devtag_usb_scan(result,sel);
}
