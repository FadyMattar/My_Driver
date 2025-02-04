#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

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

void publisher_process() {
    int pub_fd = open(DEVICE_PATH, O_RDWR);
    assert_test(pub_fd >= 0, "Publisher: Open device");
    assert_test(ioctl(pub_fd, SET_TYPE, TYPE_PUB) == 0, "Publisher: Set type");

    char write_buf[1001];
    memset(write_buf, 'A', 1001);
    int ret = write(pub_fd, write_buf, 1001);
    assert_test(ret == -1 && errno == EINVAL, "Publisher: Write 1001 bytes returns EINVAL");

    memset(write_buf, 'B', 900);
    ret = write(pub_fd, write_buf, 900);
    assert_test(ret == 900, "Publisher: Write 900 bytes");

    memset(write_buf, 'C', 101);
    ret = write(pub_fd, write_buf, 101);
    assert_test(ret == -1 && errno == EAGAIN, "Publisher: Write 101 bytes returns EAGAIN");

    sleep(6); // Allow subscribers to process

    memset(write_buf, 'D', 101);
    ret = write(pub_fd, write_buf, 101);
    assert_test(ret == 101, "Publisher: Write 101 bytes after all subscribers finished");

    close(pub_fd);
    exit(EXIT_SUCCESS);
}

void subscriber_b_process() {
    int sub_b_fd = open(DEVICE_PATH, O_RDWR);
    assert_test(sub_b_fd >= 0, "Subscriber B: Open device");
    assert_test(ioctl(sub_b_fd, SET_TYPE, TYPE_SUB) == 0, "Subscriber B: Set type");

    sleep(1); // Wait for the publisher's first write

    char read_buf[1000];
    int ret = read(sub_b_fd, read_buf, 1000);
    assert_test(ret == 900, "Subscriber B: Read 900 bytes");

    ret = read(sub_b_fd, read_buf, 1000);
    assert_test(ret == -1 && errno == EAGAIN, "Subscriber B: Second read returns EAGAIN");

    sleep(6); // Wait for the publisher's second write

    ret = read(sub_b_fd, read_buf, 200);
    assert_test(ret == 101, "Subscriber B: Read 101 bytes after second write");

    ret = read(sub_b_fd, read_buf, 200);
    assert_test(ret == -1 && errno == EAGAIN, "Subscriber B: Third read returns EAGAIN");

    close(sub_b_fd);
    exit(EXIT_SUCCESS);
}

void subscriber_c_process() {
    int sub_c_fd = open(DEVICE_PATH, O_RDWR);
    assert_test(sub_c_fd >= 0, "Subscriber C: Open device");
    assert_test(ioctl(sub_c_fd, SET_TYPE, TYPE_SUB) == 0, "Subscriber C: Set type");

    sleep(5); // Wait for the publisher's first write

    char read_buf[1000];
    int ret = read(sub_c_fd, read_buf, 500);
    assert_test(ret == 500, "Subscriber C: Read 500 bytes");

    ret = read(sub_c_fd, read_buf, 1000);
    assert_test(ret == 400, "Subscriber C: Read remaining 400 bytes");

    ret = read(sub_c_fd, read_buf, 1000);
    assert_test(ret == -1 && errno == EAGAIN, "Subscriber C: Third read returns EAGAIN");

    sleep(6); // Wait for the publisher's second write

    ret = read(sub_c_fd, read_buf, 200);
    assert_test(ret == 101, "Subscriber C: Read 101 bytes after second write");

    ret = read(sub_c_fd, read_buf, 200);
    assert_test(ret == -1 && errno == EAGAIN, "Subscriber C: Third read returns EAGAIN");

    close(sub_c_fd);
    exit(EXIT_SUCCESS);
}

int main() {
    pid_t pub_pid = fork();
    assert_test(pub_pid >= 0, "Fork for publisher process");
    if (pub_pid == 0) {
        publisher_process();
    }

    pid_t sub_b_pid = fork();
    assert_test(sub_b_pid >= 0, "Fork for subscriber B process");
    if (sub_b_pid == 0) {
        subscriber_b_process();
    }

    pid_t sub_c_pid = fork();
    assert_test(sub_c_pid >= 0, "Fork for subscriber C process");
    if (sub_c_pid == 0) {
        subscriber_c_process();
    }

    // Parent process waits for all child processes
    waitpid(pub_pid, NULL, 0);
    waitpid(sub_b_pid, NULL, 0);
    waitpid(sub_c_pid, NULL, 0);

    printf(GREEN "\nAll tests passed successfully without synchronization pipes!\n" RESET);
    return 0;
}