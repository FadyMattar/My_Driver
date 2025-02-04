#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define DEVICE_PATH "/dev/pubsub"
#define BUFFER_SIZE 1000
#define PUB_TYPE 1
#define SUB_TYPE 2
#define SET_TYPE  _IO('r', 0)
#define GET_TYPE  _IO('r', 1)

#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

// Test counter
static int test_count = 0;
static int pass_count = 0;

void assert_test(int condition, const char *test_description) {
    test_count++;
    if (condition) {
        pass_count++;
        printf(GREEN "[PASS %d] " RESET "%s\n", test_count, test_description);
    } else {
        printf(RED "[FAIL %d] %s\n", test_count, test_description);
        printf("Error: %s (errno=%d)\n" RESET, strerror(errno), errno);
        exit(EXIT_FAILURE);
    }
}

void test_suite_banner(const char *name) {
    printf(YELLOW "\n=== %s ===\n" RESET, name);
}

void test_type_setting() {
    test_suite_banner("Testing Type Setting and Validation");
    
    int fd = open(DEVICE_PATH, O_RDWR);
    assert_test(fd >= 0, "Open device");

    // Test initial state
    int type = ioctl(fd, GET_TYPE, 0);
    assert_test(type == 0, "Initial type should be TYPE_NONE");

    // Test invalid type
    int ret = ioctl(fd, SET_TYPE, 999);
    assert_test(ret == -1 && errno == EINVAL, "Setting invalid type should fail");

    // Test valid pub type
    ret = ioctl(fd, SET_TYPE, PUB_TYPE);
    assert_test(ret == 0, "Setting valid PUB_TYPE");

    // Verify type was set
    type = ioctl(fd, GET_TYPE, 0);
    assert_test(type == PUB_TYPE, "Verify type is PUB_TYPE");

    // Try to change type (should fail)
    ret = ioctl(fd, SET_TYPE, SUB_TYPE);
    assert_test(ret == -1 && (errno == EINVAL || errno == EPERM), 
                "Changing type should fail");

    close(fd);
}

void test_buffer_limits() {
    test_suite_banner("Testing Buffer Limits");
    
    int fd = open(DEVICE_PATH, O_RDWR);
    assert_test(fd >= 0, "Open device");

    // Set as publisher
    assert_test(ioctl(fd, SET_TYPE, PUB_TYPE) == 0, "Set as publisher");

    // Test writing exactly BUFFER_SIZE
    char *big_buffer = malloc(BUFFER_SIZE);
    memset(big_buffer, 'A', BUFFER_SIZE);
    
    int ret = write(fd, big_buffer, BUFFER_SIZE);
    assert_test(ret == BUFFER_SIZE, "Write exactly BUFFER_SIZE bytes");

    // Test writing more than BUFFER_SIZE
    ret = write(fd, big_buffer, BUFFER_SIZE + 1);
    assert_test(ret == -1 && errno == EINVAL, "Write > BUFFER_SIZE should fail");

    free(big_buffer);
    close(fd);
}

void test_pub_sub_interaction() {
    test_suite_banner("Testing Publisher-Subscriber Interaction");

    int pub_fd = open(DEVICE_PATH, O_RDWR);
    int sub_fd = open(DEVICE_PATH, O_RDWR);
    
    // Setup publisher and subscriber
    assert_test(ioctl(pub_fd, SET_TYPE, PUB_TYPE) == 0, "Set publisher");
    assert_test(ioctl(sub_fd, SET_TYPE, SUB_TYPE) == 0, "Set subscriber");

    // Test reading when no data available
    char read_buf[100];
    int ret = read(sub_fd, read_buf, sizeof(read_buf));
    assert_test(ret == -1 && errno == EAGAIN, "Read with no data should return EAGAIN");

    // Write test data
    const char *test_msg = "Test Message";
    size_t msg_len = strlen(test_msg);
    ret = write(pub_fd, test_msg, msg_len);
    assert_test(ret == msg_len, "Write test message");

    // Read data
    memset(read_buf, 0, sizeof(read_buf));
    ret = read(sub_fd, read_buf, sizeof(read_buf));
    assert_test(ret == msg_len, "Read should return written length");
    assert_test(memcmp(read_buf, test_msg, msg_len) == 0, "Data integrity check");

    close(pub_fd);
    close(sub_fd);
}

void test_error_conditions() {
    test_suite_banner("Testing Error Conditions");

    int fd = open(DEVICE_PATH, O_RDWR);
    assert_test(fd >= 0, "Open device");

    // Test read/write without setting type
    char buffer[10] = "test";
    int ret = write(fd, buffer, sizeof(buffer));
    assert_test(ret == -1 && errno == EPERM, "Write without type should fail");

    ret = read(fd, buffer, sizeof(buffer));
    assert_test(ret == -1 && errno == EPERM, "Read without type should fail");

    // Set as subscriber and try to write
    assert_test(ioctl(fd, SET_TYPE, SUB_TYPE) == 0, "Set as subscriber");
    ret = write(fd, buffer, sizeof(buffer));
    assert_test(ret == -1 && errno == EPERM, "Write as subscriber should fail");

    // Test invalid ioctl command
    ret = ioctl(fd, 9999, 0);
    assert_test(ret == -1 && errno == ENOTTY, "Invalid ioctl command should fail");

    close(fd);

    // Test operations on closed fd
    ret = write(fd, buffer, sizeof(buffer));
    assert_test(ret == -1 && errno == EBADF, "Write to closed fd should fail");
}

void stress_test() {
    test_suite_banner("Testing stress_test ");

    int pub_fd = open(DEVICE_PATH, O_RDWR);
    int sub_fd = open(DEVICE_PATH, O_RDWR);

    assert_test(ioctl(pub_fd, SET_TYPE, PUB_TYPE) == 0, "Set publisher");
    assert_test(ioctl(sub_fd, SET_TYPE, SUB_TYPE) == 0, "Set subscriber");

    char buffer[BUFFER_SIZE];
    const int iterations = 1000;

    // Rapid write/read cycles
    int i;
    for (i = 0; i < iterations; i++) {
        snprintf(buffer, sizeof(buffer), "Message %d", i);
        int write_ret = write(pub_fd, buffer, strlen(buffer));
        assert_test(write_ret >= 0, "Stress write");

        char read_buf[BUFFER_SIZE];
        int read_ret = read(sub_fd, read_buf, sizeof(read_buf));
        assert_test(read_ret >= 0, "Stress read");
        assert_test(memcmp(read_buf, buffer, strlen(buffer)) == 0, "Stress data integrity");
    }

    close(pub_fd);
    close(sub_fd);
}

int main() {
    srand(time(NULL));
    printf("Starting Comprehensive PubSub Driver Test\n");
    printf("=======================================\n");

    test_type_setting();
    test_buffer_limits();
    test_pub_sub_interaction();
    test_error_conditions();
    stress_test();

    printf(GREEN "\nTest Summary: %d/%d tests passed\n" RESET, pass_count, test_count);
    return 0;
}
