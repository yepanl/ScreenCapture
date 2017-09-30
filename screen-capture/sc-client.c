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

#define HOST "192.168.31.58"
#define PORT 5899

#define IMAGE_PATH "./images/screen-bmp.raw"
#define IMAGE_BMP  "./images/screen.bmp"

static int g_sock = -1;
static char *screen = NULL;
static unsigned int screen_size = 0;

static int g_dev_fd = -1;
#define ABS_DEV_PATH "/dev/fb0"

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

static void write_fb(void)
{
    int write_len = 0;
    int tmp_len = 0;

    while (1) {
        if (write_len < screen_size &&
            (tmp_len = write(g_dev_fd, screen + write_len, screen_size - write_len)) > 0)
            write_len += tmp_len;

        printf("write_len=%d\n", write_len);
    }
}

int main(int argc, const char *argv[])
{
    int addr_size;
    struct sockaddr_in addr;
    int tmp_len = 0;
    int head_len = 0;
    
    int context_len = 0;

    // open fb device
    if ((g_dev_fd = open(ABS_DEV_PATH, O_WRONLY)) < 0) {
        printf("open dev(%s) failed! errno=%d\n", ABS_DEV_PATH, errno);
        return 0;
    }

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
        //save_image();
        write_fb();
    } else {
        printf("recv() failed! recv_len=%d, errno=%d\n", screen_size, errno);
        close(g_sock);
    }

    return 0;
}



