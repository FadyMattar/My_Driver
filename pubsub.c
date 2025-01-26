/* chat.c: Example char device module.
 *
 */
/* Kernel Programming */
#define MODULE
#define LINUX
#define __KERNEL__

#include <linux/kernel.h>  	
#include <linux/module.h>
#include <linux/fs.h>       		
#include <asm/uaccess.h>
#include <linux/errno.h>  
#include <asm/segment.h>
#include <asm/current.h>
#include <linux/list.h>

#include "pubsub.h"

#define MY_DEVICE "pubsub"

MODULE_AUTHOR("Anonymous");
MODULE_LICENSE("GPL");

#define MINOR_NUM 256
#define BUFFER_SIZE 1000

/* globals */
int my_major = 0; /* will hold the major # of my device driver */

struct file_operations my_fops = {
    .open = my_open,
    .release = my_release,
    .read = my_read,
	.write = my_write,
    .ioctl = my_ioctl,
};


struct pdp_strct {
    int minor_id;
    unsigned long type;
    int seek;
    int is_writer;
    struct list_head list;
};

struct buffer_struct {
    int publisher_exists;
    int sub_counter;
    int finished_sub;
    int buff_len;
    char *buff;
    struct pdp_strct *writer_pdp;
    struct list_head *ins_pdp;
    int reference_count;
};

struct buffer_struct *buffer_array[MINOR_NUM];

int init_module(void)
{
    // This function is called when inserting the module using insmod

    my_major = register_chrdev(my_major, MY_DEVICE, &my_fops);

    if (my_major < 0)
    {
	printk(KERN_WARNING "can't get dynamic major\n");
	return my_major;
    }

    // we initialize the buffer array pointers:
    for ( int i=0 ; i < MINOR_NUM ; i++) {
        buffer_array[i] = malloc(sizeof(buffer_struct));
        if (buff_p == NULL) { return -ENOMEM;}
        buffer_array[i]->publisher_exists = 0;
        buffer_array[i]->sub_counter = 0;
        buffer_array[i]->finished_sub
        buffer_array[i]->buff_len = 0;
        buffer_array[i]->buff = NULL;
        buffer_array[i]->writer_pdp = NULL;
        INIT_LIST_HEAD(&buffer_array[i]->ins_pdp);
        buffer_array[i]->reference_count = 0;
    }

    return 0;
}


void cleanup_module(void)
{
    // This function is called when removing the module using rmmod

    unregister_chrdev(my_major, MY_DEVICE);

    for ( int i=0 ; i < MINOR_NUM ; i++) {
        free(buffer_array[i]);
    }
    return;
}


int my_open(struct inode *inode, struct file *filp)
{
    //init pdp pointer and put in filp private_data:
    struct pdp_strct *p = malloc ( sizeof (struct pdp_struct));
    if (buff_p == NULL) { return -ENOMEM;}
    p->minor_id = MINOR(inode->i_rdev);
    p->type = TYPE_NONE;
    p->seek = 0 ; 
    filp->private_data = p;
  
    if (buffer_array[p->minor_id]->buff == NULL) {
        char *buff_p = malloc ( sizeof(char)*BUFFER_SIZE);
        if (buff_p == NULL) { return -ENOMEM;}
        buffer_array[p->minor_id]->buff = buff_p;
    }

    // Check if there is a writer 
    if ( (filp -> f_mode == FMODE_WRITE) && (buffer_array[p->minor_id].publisher_exists) ) {
        -EFAULT;
    }

    list_add( &p, &buffer_array[p->minor_id]->ins_pdp);
    
    return 0;
}

int my_release(struct inode *inode, struct file *filp)
{
    struct pdp_strct *pdp_p = (struct pdp_strct)filp->private_data; 
    int minor = pdp_p->minor_id;
    
    free(&pdp_p);
    
    buffer_array[minor]->reference_count -=1;

    if( reference count == 0 ) {
        free(buffer_array[minor]->buff);
        buffer_array[minor]->publisher_exists = 0;
        buffer_array[minor]->sub_counter = 0;
        buffer_array[minor]->finished_sub
        buffer_array[minor]->buff_len = 0;
        buffer_array[minor]->buff = NULL;
        buffer_array[minor]->writer_pdp = NULL;
        INIT_LIST_HEAD(&buffer_array[minor]->ins_pdp);
        buffer_array[minor]->reference_count = 0;
    }
    
    return 0;
}

ssize_t my_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    //find minor
    struct pdp_strct *pdp_p = (struct pdp_strct)filp->private_data; 
    int minor = pdp_p->minor_id;
    int *seek = &pdp_p->seek;
    //check type
    if (pdp_p->type != TYPE_SUB) {
        return -EPERM;
    }
    
    //check if the buffer of the file exists
    if (buffer_array[minor]->buff == NULL) {
        return -EFAULT;
    }

    // find how much to read
    int read_count = buffer_array[minor]->buff_len - *seek;
    if (read_count == 0) {
        return -EAGAIN;
    }
    if (count < read_count) {
        read_count = count;
    }

    // copy to the reader buffer
    if (copy_to_user ( buf, buffer_array[minor]->buff, read_count)) {
        return -EBADF;
    }    

    // update seek according to the amount read.
    *seek += read_count;
    if ( *seek == buffer_array[minor]->buff_len) {
        buffer_array[minor]->finished_sub += 1;
    }
    // check if all subs are done reading
    if( buffer_array[minor]->finished_sub ==  buffer_array[minor]->sub_counter) {
        buffer_array[minor]->writer_pdp->is_writer = 0;
        buffer_array[minor]->publisher_exists = 0;
        buffer_array[minor]->writer_pdp = NULL;
        struct pdp_strct *entry;
        list_for_each_entry( entry, &buffer_array[minor]->ins_pdp , list) {
            entry->seek = 0 ; 
        }
    }

    return read_count; 
}

ssize_t my_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    //find minor
    struct pdp_strct *pdp_p = (struct pdp_strct)filp->private_data; 
    int minor = pdp_p->minor_id;

    //check type
    if (pdp_p->type != TYPE_PUB) {
        return -EPERM;
    }

    //check inside buffer size 
    if (count > BUFFER_SIZE) {
        return -EINVAL;
    }

    //check remaining space
    int remaining_buffer_spcae = BUFFER_SIZE - buffer_array[minor]->buff_len;
    if (count > remaining_buffer_spcae ) {
        return -EAGAIN;
    }

    //check if the buffer of the file exists
    if (buffer_array[minor]->buff == NULL) {
        return -EFAULT;
    }

    // check if i am the only writer 
    if (buffer_array[minor]->publisher_exists && !pdp_p->is_writer) {
        return -EFAULT;
    }

    //update 
    if ( !buffer_array[minor]->publisher_exists ) {
        buffer_array[minor]->publisher_exists = 1; 
        pdp_p->is_writer = 1;
        buffer_array[minor]->writer_pdp = pdp_p;
        buffer_array[minor]->buff_len = 0;
        f_pos = 0;
    }

    //copy from user to our buffer
    if ( copy_from_user(buffer_array[minor]->buff + buffer_array[minor]->buff_len,buf,count) ) {
        return -EBADF;
    }
    buffer_array[minor]->buff_len += count;

    return count;
}



int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct pdp_strct *pdp_p = (struct pdp_strct) filp->private_data;
    switch(cmd)
    {
    case SET_TYPE:
        if ((arg !=TYPE_PUB) && (arg != TYPE_SUB) ) {
            return -EINVAL;
        }
        if (pdp_p->type != TYPE_NONE) {
            return -EPERM;
        }
        pdp_p->type = arg;
        return 0;
	break;
    case GET_TYPE:
        return pdp_p->type;
	break;
    default:
	return -ENOTTY;
    }

    return 0;
}
