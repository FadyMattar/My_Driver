//
// Created by Sarim on 18/01/2025.
//
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#define DEVICE_PATH "/dev/pubsub"
#define BUFFER_SIZE 1000
#define PUB_TYPE 1
#define SUB_TYPE 2
#define SET_TYPE  _IO('r', 0)
#define GET_TYPE  _IO('r', 1)

#define GREEN "\033[32m"
#define RED "\033[31m"
#define RESET "\033[0m"

// Assertion helper to show results in green or red
void assert_test(int condition, const char *test_description) {
    if (condition) {
        printf(GREEN "PASS: %s\n" RESET, test_description);
    } else {
        printf(RED "FAIL: %s\n" RESET, test_description);
        exit(EXIT_FAILURE);
    }
}

// Test write with expected result
void test_write(int fd, const char *message, size_t size, int expected_errno, const char *test_description) {
    ssize_t ret = write(fd, message, size);
    if (ret < 0) {
        assert_test(errno == expected_errno, test_description);
    } else {
        assert_test(ret == size, test_description);
    }
}

// Test read with expected result
void test_read(int fd, size_t size, int expected_errno, const char *test_description) {
    char buffer[BUFFER_SIZE];
    ssize_t ret = read(fd, buffer, size);
    if (ret < 0) {
        assert_test(errno == expected_errno, test_description);
    } else {
        assert_test(ret > 0, test_description);
    }
}

// Test ioctl with expected result
void test_ioctl(int fd, unsigned int cmd, unsigned long arg, int expected_errno, const char *test_description) {
    int ret = ioctl(fd, cmd, arg);
    if (expected_errno != 0) {
        assert_test(ret == -1 && errno == expected_errno, test_description);
    } else {
        assert_test(ret == 0, test_description);
    }
}

int main() {
    int pub_fd, sub_fd;

    // Test Publisher Operations
    // Open the device for publisher
    pub_fd = open(DEVICE_PATH, O_RDWR);
    assert_test(pub_fd >= 0, "Open device for publisher");

    // Set type to PUB_TYPE
    test_ioctl(pub_fd, SET_TYPE, PUB_TYPE, 0, "Set type to PUB_TYPE");

    // Try to change type - should fail
    test_ioctl(pub_fd, SET_TYPE, SUB_TYPE, EPERM, "Attempt to change type (should fail)");

    // Attempt writing a message larger than BUFFER_SIZE
    test_write(pub_fd, "X", BUFFER_SIZE + 1, EINVAL, "Write message larger than BUFFER_SIZE");

    // Write a valid message
    test_write(pub_fd, "Hello, world!", 13, 0, "Write valid message");

    // Fill the buffer
    int i;
    for (i = 0; i < BUFFER_SIZE - 13; i++) {
        write(pub_fd, "X", 1); // Fill remaining space
    }
    test_write(pub_fd, "Y", 1, EAGAIN, "Write to a full buffer");

    // Test Subscriber Operations
    // Open new file descriptor for subscriber
    sub_fd = open(DEVICE_PATH, O_RDWR);
    assert_test(sub_fd >= 0, "Open device for subscriber");

    // Set type to SUB_TYPE
    test_ioctl(sub_fd, SET_TYPE, SUB_TYPE, 0, "Set type to SUB_TYPE");

    // Attempt reading valid data
    test_read(sub_fd, 50, 0, "Read valid data");

    // Attempt reading when no data is available
    test_read(sub_fd, 50, EAGAIN, "Read with no data available");

    // Attempt writing as SUB_TYPE
    test_write(sub_fd, "Invalid write", 14, EPERM, "Write as SUB_TYPE");

    // Test uninitialized device
    int new_fd = open(DEVICE_PATH, O_RDWR);
    assert_test(new_fd >= 0, "Open device again");
    test_read(new_fd, 50, EPERM, "Read without setting SUB_TYPE");

    // Set type to SUB_TYPE and verify type with GET_TYPE
    int type_result = 0;
    test_ioctl(new_fd, SET_TYPE, SUB_TYPE, 0, "Set type to SUB_TYPE");
    test_ioctl(new_fd, GET_TYPE, (unsigned long)&type_result, 0, "Get current type");
    printf("type_result = %d\n", type_result);
    assert_test(type_result == SUB_TYPE, "Verify current type is SUB_TYPE");

    // Close all devices
    assert_test(close(pub_fd) == 0, "Close publisher device");
    assert_test(close(sub_fd) == 0, "Close subscriber device");
    assert_test(close(new_fd) == 0, "Close test device");

    printf(GREEN "All tests passed successfully!\n" RESET);
    return 0;
}