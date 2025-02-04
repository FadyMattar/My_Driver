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

void assert_test(int condition, const char *test_description) {
    if (condition) {
        printf(GREEN "PASS: %s\n" RESET, test_description);
    } else {
        printf(RED "FAIL: %s\n" RESET, test_description);
        printf(RED "Detailed error: %s (errno=%d)\n" RESET, strerror(errno), errno);
        exit(EXIT_FAILURE); // Stop the test suite immediately
    }
}

// Test suite 1: Basic device operations
void test_basic_operations() {
    int fd = open(DEVICE_PATH, O_RDWR);
    assert_test(fd >= 0, "Open device");

    // Test initial state
    int type = ioctl(fd, GET_TYPE, 0);
    assert_test(type == 0, "Initial type should be NONE");

    close(fd);
}

// Test suite 2: Publisher operations
void test_publisher_operations() {
    int fd = open(DEVICE_PATH, O_RDWR);
    char buffer[BUFFER_SIZE + 1];

    // Set type to publisher
    int ret = ioctl(fd, SET_TYPE, PUB_TYPE);
    assert_test(ret == 0, "Set type to PUB_TYPE");

    // Try writing more than buffer size
    memset(buffer, 'X', BUFFER_SIZE + 1);
    ret = write(fd, buffer, BUFFER_SIZE + 1);
    assert_test(ret == -1 && errno == EINVAL, "Write larger than BUFFER_SIZE");
	
	
	printf("Type is : %d\n", ioctl(fd, GET_TYPE, 0));
    // Write valid message
    ret = write(fd, "Hello", 5);
	printf(" ret is : %d \n", ret);
    assert_test(ret == 5, "Write valid message");

    close(fd);
}

// Test suite 3: Subscriber operations
void test_subscriber_operations() {
    int fd = open(DEVICE_PATH, O_RDWR);
    char buffer[BUFFER_SIZE];

    // Set type to subscriber
    int ret = ioctl(fd, SET_TYPE, SUB_TYPE);
    assert_test(ret == 0, "Set type to SUB_TYPE");

    // Try reading when no data
    ret = read(fd, buffer, BUFFER_SIZE);
    assert_test(ret == -1 && errno == EAGAIN, "Read with no data available");

    close(fd);
}

// Test suite 4: Publisher-Subscriber interaction
void test_pub_sub_interaction() {
    int pub_fd, sub_fd;
    char buffer[BUFFER_SIZE];
    int ret;

    // Open publisher and set type
    pub_fd = open(DEVICE_PATH, O_RDWR);
    assert_test(pub_fd >= 0, "Open publisher device");
    ret = ioctl(pub_fd, SET_TYPE, PUB_TYPE);
    assert_test(ret == 0, "Set type to PUB_TYPE");

    // Open subscriber and set type
    sub_fd = open(DEVICE_PATH, O_RDWR);
    assert_test(sub_fd >= 0, "Open subscriber device");
    ret = ioctl(sub_fd, SET_TYPE, SUB_TYPE);
    assert_test(ret == 0, "Set type to SUB_TYPE");

    // Write message from publisher
    const char *message = "Test Message";
    ret = write(pub_fd, message, strlen(message));
    assert_test(ret == strlen(message), "Publisher write message");

    // Clear buffer before reading
    memset(buffer, 0, BUFFER_SIZE);

    // Read message with exact length
    ret = read(sub_fd, buffer, strlen(message));
    assert_test(ret == strlen(message), "Subscriber read message");

    // Verify message content
    assert_test(memcmp(buffer, message, strlen(message)) == 0, "Message content matches");

    close(sub_fd);
    close(pub_fd);
}

// Test suite 5: Error conditions
void test_error_conditions() {
    int fd = open(DEVICE_PATH, O_RDWR);
    assert_test(fd >= 0, "Open device for error conditions");

    // Try reading without setting type
    char buffer[BUFFER_SIZE];
    int ret = read(fd, buffer, BUFFER_SIZE);
    assert_test(ret == -1 && errno == EPERM, "Read without setting type");

    // Try writing without setting type
    ret = write(fd, "test", 4);
    assert_test(ret == -1 && errno == EPERM, "Write without setting type");

    // Try setting an invalid type
    ret = ioctl(fd, SET_TYPE, 999);
    assert_test(ret == -1 && errno == EINVAL, "Set invalid type");

    // Set type and try setting it again
    ret = ioctl(fd, SET_TYPE, PUB_TYPE);
    assert_test(ret == 0, "Set type to PUB_TYPE");
    ret = ioctl(fd, SET_TYPE, SUB_TYPE);
    assert_test(ret == -1 && errno == EPERM, "Set type twice");

    close(fd);
}

int main() {
    printf("\nRunning PubSub Device Driver Tests\n");
    printf("===================================\n\n");

    test_basic_operations();
    test_publisher_operations();
    test_subscriber_operations();
    test_pub_sub_interaction();
    test_error_conditions();

    printf(GREEN "\nAll tests passed successfully!\n" RESET);
    return 0;
}
