#ifndef DEVTAG_H
#define DEVTAG_H

#include <sys/types.h>

/*
 * Lookup 'devtag' among the predefined device selections.
 * If a matching device is found return the name of this device in buf.
 *
 * If 'devtag' is prefixed with "/dev/" then the result is also
 * prefixed with "/dev/".
 *
 * Returns number of devices found or a negative value if an error occured.
 */
int devtag_lookup(char *buf, size_t bufsize, const char *devtag);

/* also returns a constant name if supplied in the configuration */
int devtag_lookup2(char *buf, size_t bufsize, char *constbuf, size_t constsize, const char *devtag);

/*
 * Simple lookup.
 * Returns a suitable replacement for devtag.
 * If no replacement can be determined a 'devtag' will be returned as a duplicated string.
 */
char *devtag_get(const char *devtag);

#endif
