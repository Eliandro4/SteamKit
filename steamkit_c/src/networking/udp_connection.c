#define _DEFAULT_SOURCE
#include "steamkit/networking/udp_connection.h"
#include "steamkit/networking/connection.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define UDP_RECV_BUF_SIZE 65536

struct sk_udp_connection {
    sk_connection_t base;
    int sockfd;
    char host[256];
    uint16_t port;
    bool connected;
    bool running;
    pthread_t recv_thread;
    uint8_t recv_buf[UDP_RECV_BUF_SIZE];
    size_t recv_len;
    pthread_mutex_t recv_mutex;
};

static void* udp_recv_thread(void* arg) {
    sk_udp_connection_t* udp = (sk_udp_connection_t*)arg;
    if (!udp) return NULL;

    while (udp->running) {
        if (udp->sockfd < 0) break;

        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        ssize_t n = recvfrom(udp->sockfd,
                             udp->recv_buf + udp->recv_len,
                             UDP_RECV_BUF_SIZE - udp->recv_len - 1,
                             0,
                             (struct sockaddr*)&from_addr,
                             &from_len);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                usleep(10000);
                continue;
            }
            if (udp->running) {
                udp->connected = false;
                if (udp->base.disconnected_callback) {
                    udp->base.disconnected_callback(udp->base.user_data, true);
                }
            }
            break;
        }

        pthread_mutex_lock(&udp->recv_mutex);
        udp->recv_len += (size_t)n;
        pthread_mutex_unlock(&udp->recv_mutex);

        if (udp->base.net_msg_callback) {
            pthread_mutex_lock(&udp->recv_mutex);
            udp->base.net_msg_callback(udp->base.user_data,
                                       udp->recv_buf,
                                       udp->recv_len,
                                       SK_EMSG_INVALID);
            udp->recv_len = 0;
            pthread_mutex_unlock(&udp->recv_mutex);
        }
    }

    return NULL;
}

sk_udp_connection_t* sk_udp_connection_create(void) {
    sk_udp_connection_t* udp = (sk_udp_connection_t*)calloc(1, sizeof(sk_udp_connection_t));
    if (udp) {
        udp->base.protocol = SK_PROTOCOL_TYPE_UDP;
        udp->base.impl = udp;
        udp->sockfd = -1;
        pthread_mutex_init(&udp->recv_mutex, NULL);
    }
    return udp;
}

void sk_udp_connection_connect(sk_udp_connection_t* udp, const char* host, uint16_t port, int timeout_ms) {
    if (!udp || !host) return;
    if (udp->connected) return;

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);

    int gai = getaddrinfo(host, port_str, &hints, &res);
    if (gai != 0 || !res) return;

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        return;
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        close(sockfd);
        freeaddrinfo(res);
        return;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(sockfd);
        freeaddrinfo(res);
        return;
    }

    if (timeout_ms > 0) {
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    udp->sockfd = sockfd;
    udp->connected = true;
    udp->base.is_connected = true;
    strncpy(udp->host, host, sizeof(udp->host) - 1);
    udp->port = port;

    freeaddrinfo(res);

    udp->running = true;
    if (pthread_create(&udp->recv_thread, NULL, udp_recv_thread, udp) != 0) {
        close(udp->sockfd);
        udp->sockfd = -1;
        udp->connected = false;
        udp->base.is_connected = false;
        udp->running = false;
        return;
    }

    if (udp->base.connected_callback) {
        udp->base.connected_callback(udp->base.user_data);
    }
}

void sk_udp_connection_disconnect(sk_udp_connection_t* udp, bool user_initiated) {
    if (!udp) return;

    bool was_connected = udp->connected;
    udp->running = false;
    udp->connected = false;
    udp->base.is_connected = false;

    if (udp->sockfd >= 0) {
        close(udp->sockfd);
        udp->sockfd = -1;
    }

    if (udp->recv_thread) {
        pthread_join(udp->recv_thread, NULL);
        udp->recv_thread = (pthread_t)0;
    }

    if (udp->base.disconnected_callback && was_connected) {
        udp->base.disconnected_callback(udp->base.user_data, user_initiated);
    }
}

void sk_udp_connection_send(sk_udp_connection_t* udp, const uint8_t* data, size_t len) {
    if (!udp || !data || len == 0 || udp->sockfd < 0) return;

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(udp->port);
    inet_pton(AF_INET, udp->host, &dest_addr.sin_addr);

    ssize_t sent = sendto(udp->sockfd, data, len, 0,
                          (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (sent < 0) {
        udp->connected = false;
        if (udp->base.disconnected_callback) {
            udp->base.disconnected_callback(udp->base.user_data, true);
        }
    }
}

void sk_udp_connection_bind(sk_udp_connection_t* udp, uint16_t port) {
    if (!udp || udp->sockfd >= 0) return;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return;

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return;
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        close(sockfd);
        return;
    }
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    udp->sockfd = sockfd;
    udp->connected = true;
    udp->base.is_connected = true;

    udp->running = true;
    if (pthread_create(&udp->recv_thread, NULL, udp_recv_thread, udp) != 0) {
        close(udp->sockfd);
        udp->sockfd = -1;
        udp->connected = false;
        udp->base.is_connected = false;
        udp->running = false;
        return;
    }
}

ssize_t sk_udp_connection_recv(sk_udp_connection_t* udp, uint8_t* buf, size_t buf_len, int timeout_ms) {
    if (!udp || !buf || buf_len == 0 || udp->sockfd < 0) return -1;

    if (timeout_ms > 0) {
        fd_set fds;
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        FD_ZERO(&fds);
        FD_SET(udp->sockfd, &fds);
        int rc = select(udp->sockfd + 1, &fds, NULL, NULL, &tv);
        if (rc <= 0) return -1;
    }

    return recv(udp->sockfd, buf, buf_len, 0);
}
