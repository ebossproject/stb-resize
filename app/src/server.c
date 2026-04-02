#include "editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

// Helper: read exactly n bytes from fd
static int read_n(int fd, void *buf, size_t n) {
    size_t got = 0;
    while (got < n) {
        ssize_t ret = read(fd, (char*)buf + got, n - got);
        if (ret <= 0) return -1;
        got += ret;
    }
    return 0;
}

// Helper: write exactly n bytes to fd
static int write_n(int fd, const void *buf, size_t n) {
    size_t sent = 0;
    while (sent < n) {
        ssize_t ret = write(fd, (const char*)buf + sent, n - sent);
        if (ret <= 0) return -1;
        sent += ret;
    }
    return 0;
}

int main(int argc, char **argv) {
    int port = 5000;
    if (argc > 1)
        port = atoi(argv[1]);
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { perror("socket"); exit(1); }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(listenfd, 8) < 0) { perror("listen"); exit(1); }
    fprintf(stderr, "Listening on port %d...\n", port);

    // Allow quick reuse of the port after server restart:
    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    while (1) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) { perror("accept"); continue; }
        // Read 4-byte YAML length, 4-byte image length (big-endian)
        uint32_t yamllen_n, imglen_n;
        if (read_n(connfd, &yamllen_n, 4) < 0 || read_n(connfd, &imglen_n, 4) < 0) {
            close(connfd); continue;
        }
        size_t yamllen = ntohl(yamllen_n);
        size_t imglen = ntohl(imglen_n);
        if (yamllen > (1<<20) || imglen > (1<<24)) { // 1MB YAML, 16MB image
            close(connfd); continue;
        }
        unsigned char *yamlbuf = malloc(yamllen ? yamllen : 1);
        unsigned char *imgbuf = malloc(imglen ? imglen : 1);
        if (!yamlbuf || !imgbuf) { close(connfd); free(yamlbuf); free(imgbuf); continue; }
        if (read_n(connfd, yamlbuf, yamllen) < 0 || read_n(connfd, imgbuf, imglen) < 0) {
            close(connfd); free(yamlbuf); free(imgbuf); continue;
        }

        FILE *sockfile = fdopen(connfd, "wb");
        if (!sockfile) {
            close(connfd); free(yamlbuf); free(imgbuf); continue;
        }
        editor_process(imgbuf, imglen, (char*)yamlbuf, yamllen, sockfile);
        fflush(sockfile);
        fclose(sockfile); // closes connfd
        free(yamlbuf); free(imgbuf);
    }
    return 0;
}
