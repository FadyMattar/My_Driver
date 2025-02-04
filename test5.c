#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#define DEVICE_PATH "/dev/pubsub"
#define BUFFER_SIZE 1000
#define TYPE_PUB 1
#define TYPE_SUB 2
#define SET_TYPE _IO('r', 0)

#define GREEN "\033[32m"
#define RED "\033[31m"
#define RESET "\033[0m"

void assert_test(int condition, const char *test_description) {
    if (condition) {
        printf(GREEN "PASS: %s\n" RESET, test_description);
    } else {
        printf(RED "FAIL: %s\n" RESET, test_description);
        printf("Error: %s (errno=%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }
}

int main() {
    printf("Running test based on the provided example\n");

    // Open publisher device
    int pub_fd = open(DEVICE_PATH, O_RDWR);
    assert_test(pub_fd >= 0, "Open publisher device");
    assert_test(ioctl(pub_fd, SET_TYPE, TYPE_PUB) == 0, "Set publisher type");

    // Try to write 1001 bytes (should return EINVAL)
    char write_buf[1001];
    memset(write_buf, 'A', 1001);
    int ret = write(pub_fd, write_buf, 1001);
    assert_test(ret == -1 && errno == EINVAL, "Write 1001 bytes returns EINVAL");

    // Write 900 bytes successfully
    memset(write_buf, 'B', 900);
    ret = write(pub_fd, write_buf, 900);
    assert_test(ret == 900, "Write 900 bytes");

    // Open subscriber devices B and C
    int sub_b_fd = open(DEVICE_PATH, O_RDWR);
    int sub_c_fd = open(DEVICE_PATH, O_RDWR);
    assert_test(sub_b_fd >= 0 && sub_c_fd >= 0, "Open subscriber devices");
    assert_test(ioctl(sub_b_fd, SET_TYPE, TYPE_SUB) == 0, "Set subscriber B type");
    assert_test(ioctl(sub_c_fd, SET_TYPE, TYPE_SUB) == 0, "Set subscriber C type");

    // Subscriber B reads 900 bytes
    char read_buf[1000];
    ret = read(sub_b_fd, read_buf, 1000);
    assert_test(ret == 900, "Subscriber B read 900 bytes");

    // Subscriber B's second read should return EAGAIN
    ret = read(sub_b_fd, read_buf, 1000);
    assert_test(ret == -1 && errno == EAGAIN, "Subscriber B second read returns EAGAIN");

    // Subscriber C reads 500 bytes
    ret = read(sub_c_fd, read_buf, 500);
    assert_test(ret == 500, "Subscriber C read 500 bytes");

    // Subscriber C reads remaining 400 bytes
    ret = read(sub_c_fd, read_buf, 1000);
    assert_test(ret == 400, "Subscriber C read remaining 400 bytes");

    // Subscriber C's third read should return EAGAIN
    ret = read(sub_c_fd, read_buf, 1000);
    assert_test(ret == -1 && errno == EAGAIN, "Subscriber C third read returns EAGAIN");

    // Write 101 bytes after all subscribers finished
    memset(write_buf, 'C', 101);
    ret = write(pub_fd, write_buf, 101);
    assert_test(ret == 101, "Write 101 bytes after all subscribers finished");

    // Subscriber B reads 101 bytes (requesting 200)
    ret = read(sub_b_fd, read_buf, 200);
    assert_test(ret == 101, "Subscriber B read 101 bytes (request 200 bytes)");

    // Subscriber B's second read of 200 bytes returns EAGAIN
    ret = read(sub_b_fd, read_buf, 200);
    assert_test(ret == -1 && errno == EAGAIN, "Subscriber B second read of 200 bytes returns EAGAIN");

    // Subscriber C reads 101 bytes (requesting 200)
    ret = read(sub_c_fd, read_buf, 200);
    assert_test(ret == 101, "Subscriber C read 101 bytes (request 200 bytes)");

    // Subscriber C's second read of 200 bytes returns EAGAIN
    ret = read(sub_c_fd, read_buf, 200);
    assert_test(ret == -1 && errno == EAGAIN, "Subscriber C second read of 200 bytes returns EAGAIN");

    // Cleanup
    close(pub_fd);
    close(sub_b_fd);
    close(sub_c_fd);

    printf(GREEN "\nAll tests passed successfully!\n" RESET);
    return 0;
}