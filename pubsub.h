#ifndef _PUBSUB_H_
#define _PUBSUB_H_

#include <linux/ioctl.h>
#include <linux/types.h>

#define TYPE_NONE 0
#define TYPE_PUB 1
#define TYPE_SUB 2

//
// Function prototypes
//
int my_open(struct inode *, struct file *);

int my_release(struct inode *, struct file *);

ssize_t my_read(struct file *, char *, size_t, loff_t *);

int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

#define MY_MAGIC 'r'
#define SET_TYPE  _IO(MY_MAGIC, 0)
#define GET_TYPE  _IO(MY_MAGIC, 1)

#endif
