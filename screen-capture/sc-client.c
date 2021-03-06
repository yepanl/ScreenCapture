#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <errno.h>
#include <arpa/inet.h>

#define HOST "192.168.31.205"
#define PORT 7465

#define IMAGE_PATH "./images/screen.jpeg"

static int g_sock = -1;
static char *screen = NULL;
static unsigned int screen_size = 0;

static char head[4];

static void save_image(void)
{
    FILE *fp = fopen(IMAGE_PATH, "wb");
    if (!fp)
        return;

    fwrite(screen, screen_size, 1, fp);
    fclose(fp);

    return;
}

int main(int argc, const char *argv[])
{
    int addr_size;
    struct sockaddr_in addr;
    int tmp_len = 0;
    int head_len = 0;
    
    int context_len = 0;

    g_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (g_sock < 0) {
        printf("open() socket failed! errno=%d\n", errno);
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(HOST);
    addr.sin_port = htons(PORT);

    addr_size = sizeof(addr);
    if(connect(g_sock, (struct sockaddr *) &addr, addr_size) < 0) {
        printf("connect() failed! errno=%d\n", errno);
        return 0;
    }

    // receive length firstly
    while (head_len < sizeof(head) &&
           (tmp_len = recv(g_sock, head + head_len, sizeof(head) - head_len, 0)) > 0)
        head_len += tmp_len;

    if (head_len < sizeof(head)) {
        printf("Receive head failed! errno=%d\n", errno);
        return 0;
    }
    context_len = ntohl(*(unsigned int *)head);
    printf("Receive head, context_len=%d\n", context_len);

    // receive msg from server
    if (!(screen = malloc(context_len))) {
        printf("malloc() failed!\n");
        return 0;
    }
    memset(screen, 0, context_len);

    while (screen_size < context_len &&
           (tmp_len = recv(g_sock, screen + screen_size, context_len - screen_size, 0)) > 0)
        screen_size += tmp_len;

    if (screen_size == context_len) {
        printf("recv_len=%d\n", screen_size);
        close(g_sock);
        save_image();
    } else {
        printf("recv() failed! recv_len=%d, errno=%d\n", screen_size, errno);
        close(g_sock);
    }

    return 0;
}



