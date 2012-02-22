#ifndef LIBDEVTAG_H
#define LIBDEVTAG_H

#define LIBDEVTAG_VERSION "1.0.6"

struct devname;
struct devname_head {
  struct devname *head;
};

struct devname {
  char *dev;
  char *devname;
  char *type;
  int pos;
  struct devname *next;
};

struct devinfo;
struct devinfo_head {
  struct devinfo *head;
};

struct devinfo {
  const char *name;
  const char *value;
  struct devinfo *next;
};

struct dev;
struct dev_head {
  struct dev *head;
};

struct dev {
  struct devinfo_head info;
  struct devname_head devnames;
  char *class; /* "usb", "pci" etc */
  struct dev *next;
};

/*
 * Find devices matching selections in list 'sel' (of type struct dev_info).
 * Put matching devices in result list 'result'.
 */
int devtag_dev_scan(struct dev_head *result, const struct devinfo_head *sel);

/*
 * Find USB devices matching selections in list 'sel' (of type struct dev_info).
 * Put matching USB devices in result list 'result'.
 */
int devtag_usb_scan(struct dev_head *result, const struct devinfo_head *sel);


#endif
