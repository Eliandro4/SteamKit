#define _DEFAULT_SOURCE
#include "steamkit/networking/tcp_connection.h"
#include "steamkit/networking/connection.h"
#include "steamkit/base/msg_hdr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#define TCP_RECV_BUF_SIZE 65536

struct sk_tcp_connection {
    sk_connection_t base;
    int sockfd;
    char host[256];
    uint16_t port;
    bool connected;
    bool running;
    pthread_t thread;
    uint8_t recv_buf[TCP_RECV_BUF_SIZE];
    size_t recv_len;
};

static void* tcp_recv_thread(void* arg) {
    sk_tcp_connection_t* tcp = (sk_tcp_connection_t*)arg;
    if (!tcp) return NULL;

    while (tcp->running) {
        if (tcp->sockfd < 0) break;

        ssize_t n = recv(tcp->sockfd,
                          tcp->recv_buf + tcp->recv_len,
                          TCP_RECV_BUF_SIZE - tcp->recv_len - 1,
                          0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            if (tcp->running) {
                tcp->connected = false;
                if (tcp->base.disconnected_callback) {
                    tcp->base.disconnected_callback(tcp->base.user_data, true);
                }
            }
            break;
        }

        tcp->recv_len += (size_t)n;

        while (tcp->recv_len >= 24) {
            uint32_t msg_len;
            memcpy(&msg_len, tcp->recv_buf, 4);
            if (tcp->recv_len < 4U + (size_t)msg_len) break;

            const uint8_t* payload = tcp->recv_buf + 4;
            size_t payload_len = (size_t)msg_len;

            sk_emsg_t emsg = SK_EMSG_INVALID;
            if (payload_len >= 20) {
                sk_msg_hdr_t hdr;
                if (sk_msg_hdr_deserialize(&hdr, payload, payload_len)) {
                    emsg = hdr.msg;
                }
            }

            if (tcp->base.net_msg_callback) {
                tcp->base.net_msg_callback(tcp->base.user_data,
                                            payload,
                                            payload_len,
                                            emsg);
            }

            memmove(tcp->recv_buf,
                    tcp->recv_buf + 4 + (size_t)msg_len,
                    tcp->recv_len - 4U - (size_t)msg_len);
            tcp->recv_len -= 4U + (size_t)msg_len;
        }
    }

    return NULL;
}

sk_tcp_connection_t* sk_tcp_connection_create(void) {
    sk_tcp_connection_t* tcp = (sk_tcp_connection_t*)calloc(1, sizeof(sk_tcp_connection_t));
    if (!tcp) return NULL;
    tcp->base.protocol = SK_PROTOCOL_TYPE_TCP;
    tcp->base.impl = tcp;
    tcp->sockfd = -1;
    return tcp;
}

void sk_tcp_connection_destroy(sk_tcp_connection_t* tcp) {
    if (!tcp) return;
    sk_connection_disconnect((sk_connection_t*)tcp, true);
    free(tcp);
}

static int tcp_connect_internal(sk_tcp_connection_t* tcp, const char* host, uint16_t port, int timeout_ms) {
    if (!tcp || !host) return -1;

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);

    int gai = getaddrinfo(host, port_str, &hints, &res);
    if (gai != 0 || !res) return -1;

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        return -1;
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    int rc = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (rc < 0 && errno != EINPROGRESS) {
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    if (rc == 0) {
        fcntl(sockfd, F_SETFL, flags);
        tcp->sockfd = sockfd;
        tcp->connected = true;
        tcp->base.is_connected = true;
        strncpy(tcp->host, host, sizeof(tcp->host) - 1);
        tcp->port = port;
        freeaddrinfo(res);
        return 0;
    }

    fd_set wfds;
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);

    rc = select(sockfd + 1, NULL, &wfds, NULL, &tv);
    if (rc <= 0) {
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    int so_error = 0;
    socklen_t len = sizeof(so_error);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error != 0) {
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    fcntl(sockfd, F_SETFL, flags);

    tcp->sockfd = sockfd;
    tcp->connected = true;
    strncpy(tcp->host, host, sizeof(tcp->host) - 1);
    tcp->port = port;

    freeaddrinfo(res);
    return 0;
}

void sk_tcp_connection_connect(sk_tcp_connection_t* tcp, const char* host, uint16_t port, int timeout_ms) {
    if (!tcp || !host) return;
    if (tcp->connected) return;

    if (tcp_connect_internal(tcp, host, port, timeout_ms) != 0) return;

    tcp->running = true;
    if (pthread_create(&tcp->thread, NULL, tcp_recv_thread, tcp) != 0) {
        close(tcp->sockfd);
        tcp->sockfd = -1;
        tcp->connected = false;
        tcp->running = false;
        return;
    }

    if (tcp->base.connected_callback) {
        tcp->base.connected_callback(tcp->base.user_data);
    }
}

void sk_tcp_connection_disconnect(sk_tcp_connection_t* tcp, bool user_initiated) {
    if (!tcp) return;

    bool was_connected = tcp->connected;
    tcp->running = false;
    tcp->connected = false;
    tcp->base.is_connected = false;

    if (tcp->sockfd >= 0) {
        shutdown(tcp->sockfd, SHUT_RDWR);
        close(tcp->sockfd);
        tcp->sockfd = -1;
    }

    if (tcp->thread) {
        pthread_join(tcp->thread, NULL);
        tcp->thread = (pthread_t)0;
    }

    if (tcp->base.disconnected_callback && was_connected) {
        tcp->base.disconnected_callback(tcp->base.user_data, user_initiated);
    }
}

void sk_tcp_connection_send(sk_tcp_connection_t* tcp, const uint8_t* data, size_t len) {
    if (!tcp || !data || len == 0) return;

    uint8_t header[4];
    memcpy(header, &len, 4);

    ssize_t total_sent = 0;
    ssize_t ret;

    ret = send(tcp->sockfd, header, 4, 0);
    if (ret < 0) {
        if (errno == EINTR) ret = 0;
        else goto send_error;
    }
    total_sent = ret;

    while ((size_t)total_sent < len) {
        ret = send(tcp->sockfd, data + total_sent, (size_t)(len - total_sent), 0);
        if (ret < 0) {
            if (errno == EINTR) continue;
            goto send_error;
        }
        total_sent += ret;
    }
    return;

send_error:
    tcp->connected = false;
    if (tcp->base.disconnected_callback) {
        tcp->base.disconnected_callback(tcp->base.user_data, true);
    }
}
