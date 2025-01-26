#!/usr/bin/python

from __future__ import division
from struct import *
import fcntl
import struct
import os
import errno

#
# Globals
#
DEVICE_PATH = '/dev/pubsub'
TYPE_NONE = 0
TYPE_PUB = 1
TYPE_SUB = 2

#
# Utilities for calculating the IOCTL command codes.
#
sizeof = {
    'byte': calcsize('c'),
    'signed byte': calcsize('b'),
    'unsigned byte': calcsize('B'),
    'short': calcsize('h'),
    'unsigned short': calcsize('H'),
    'int': calcsize('i'),
    'unsigned int': calcsize('I'),
    'long': calcsize('l'),
    'unsigned long': calcsize('L'),
    'long long': calcsize('q'),
    'unsigned long long': calcsize('Q')
}

_IOC_NRBITS = 8
_IOC_TYPEBITS = 8
_IOC_SIZEBITS = 14
_IOC_DIRBITS = 2

_IOC_NRMASK = ((1 << _IOC_NRBITS)-1)
_IOC_TYPEMASK = ((1 << _IOC_TYPEBITS)-1)
_IOC_SIZEMASK = ((1 << _IOC_SIZEBITS)-1)
_IOC_DIRMASK = ((1 << _IOC_DIRBITS)-1)

_IOC_NRSHIFT = 0
_IOC_TYPESHIFT = (_IOC_NRSHIFT+_IOC_NRBITS)
_IOC_SIZESHIFT = (_IOC_TYPESHIFT+_IOC_TYPEBITS)
_IOC_DIRSHIFT = (_IOC_SIZESHIFT+_IOC_SIZEBITS)

_IOC_NONE = 0
_IOC_WRITE = 1
_IOC_READ = 2

def _IOC(dir, _type, nr, size):
    if type(_type) == str:
        _type = ord(_type)
        
    cmd_number = (((dir)  << _IOC_DIRSHIFT) | \
        ((_type) << _IOC_TYPESHIFT) | \
        ((nr)   << _IOC_NRSHIFT) | \
        ((size) << _IOC_SIZESHIFT))

    return cmd_number

def _IO(_type, nr):
    return _IOC(_IOC_NONE, _type, nr, 0)

def _IOR(_type, nr, size):
    return _IOC(_IOC_READ, _type, nr, sizeof[size])

def _IOW(_type, nr, size):
    return _IOC(_IOC_WRITE, _type, nr, sizeof[size])

def main():
    """Test the device driver"""
    
    #
    # Calculate the ioctl cmd number
    #
    MY_MAGIC = 'r'
    SET_TYPE = _IO(MY_MAGIC, 0)
    GET_TYPE = _IO(MY_MAGIC, 1)

    # Open the device file
    f = os.open(DEVICE_PATH, os.O_RDWR)
    
    # Check we don't have a type
    assert (fcntl.ioctl(f, GET_TYPE) == TYPE_NONE)

    # Make us a publisher
    fcntl.ioctl(f, SET_TYPE, TYPE_PUB)

    # Check we have a type
    assert (fcntl.ioctl(f, GET_TYPE) == TYPE_PUB)

    # We should not be able to read
    try:
        ret = os.read(f, 100)
        assert False # We should not reach this line
    except OSError, e:
         # Check the correct error was raised
        assert (e.errno == errno.EPERM)

    # We should be able to write
    ret = os.write(f, "hello")
    assert (ret == 5)
    
    # We should not be able to write a very large message
    try:
        ret = os.write(f, "X" * 1050)
        assert False # We should not reach this line
    except OSError, e:
         # Check the correct error was raised
        assert (e.errno == errno.EINVAL)

    # Finaly close the device file
    os.close(f)

    
if __name__ == '__main__':
    main()
    
