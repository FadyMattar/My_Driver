rm -f /dev/pubsub
rmmod pubsub
make clean
make
insmod ./pubsub.o
mknod /dev/pubsub c 254 0
# python test.py
# dmesg