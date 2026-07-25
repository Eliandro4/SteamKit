#include "steamkit/utils/net_helpers.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

uint32_t sk_net_random_uint32(void) {
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)(time(NULL) ^ getpid()));
        seeded = 1;
    }
    return (uint32_t)rand();
}

uint64_t sk_net_random_uint64(void) {
    return ((uint64_t)sk_net_random_uint32() << 32) | sk_net_random_uint32();
}

uint32_t sk_net_get_local_ip(void) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return 0x0100007F;
    
    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr("8.8.8.8");
    serv.sin_port = htons(53);
    
    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        close(sock);
        return 0x0100007F;
    }
    
    struct sockaddr_in name = {0};
    socklen_t namelen = sizeof(name);
    if (getsockname(sock, (struct sockaddr*)&name, &namelen) < 0) {
        close(sock);
        return 0x0100007F;
    }
    
    uint32_t ip = ntohl(name.sin_addr.s_addr);
    close(sock);
    return ip ? ip : 0x0100007F;
}

int sk_net_create_udp_socket(void) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct timeval tv = {1, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    return sock;
}

void sk_net_close_socket(int socket) {
    if (socket >= 0) close(socket);
}
