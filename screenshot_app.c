#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define NETLINK_USER 31
#define MAX_PAYLOAD 1024

void register_with_kernel(int sock_fd) {
    struct nlmsghdr *nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    struct sockaddr_nl dest_addr;

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; // Kernel mặc định PID là 0
    dest_addr.nl_groups = 0;

    // Thiết lập tin nhắn Netlink
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid(); // Gửi PID của chính ứng dụng
    nlh->nlmsg_flags = 0;
    strcpy(NLMSG_DATA(nlh), "register");

    // Gửi tin nhắn tới kernel
    if (sendto(sock_fd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Error sending message to kernel");
    } else {
        printf("Registered with kernel, PID sent: %d\n", getpid());
    }

    free(nlh);
}

void listen_for_netlink_messages(int sock_fd) {
    struct sockaddr_nl src_addr, dest_addr;
    struct nlmsghdr *nlh;
    struct iovec iov;
    struct msghdr msg;

    int screenshot_count = 0; // Đếm số ảnh đã chụp

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); // PID của tiến trình hiện tại

    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        perror("Error binding socket");
        return;
    }

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    iov.iov_base = (void *)nlh;
    iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    while (1) {
        if (recvmsg(sock_fd, &msg, 0) < 0) {
            perror("Error receiving message");
            continue;
        }

        printf("Received message payload: %s\n", (char *)NLMSG_DATA(nlh));

        // Thực hiện lệnh chụp ảnh nếu nhận được tín hiệu "screenshot"
        if (strcmp((char *)NLMSG_DATA(nlh), "screenshot") == 0) {
            
            char screenshot_path[256];
            sprintf(screenshot_path, "/home/dyhy/Pictures/screenshot_%d.png", screenshot_count);

            // Gọi lệnh chụp ảnh với tên tệp mới
            char command[512];
            sprintf(command, "gnome-screenshot -f %s", screenshot_path);
            system(command);

            printf("Screenshot taken and saved as: %s\n", screenshot_path);

            // Tăng bộ đếm ảnh
            screenshot_count++;
        }
    }

    free(nlh);
}

int main() {
    int sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);

    if (sock_fd < 0) {
        perror("socket creation failed");
        return -1;
    }

    // Đăng ký với kernel bằng cách gửi PID
    register_with_kernel(sock_fd);

    printf("Listening for Netlink messages...\n");
    listen_for_netlink_messages(sock_fd);

    close(sock_fd);
    return 0;
}
