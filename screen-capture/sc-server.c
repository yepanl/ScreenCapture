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
#define MAX_LISTEN_FD 256
#define ABS_DEV_PATH "/dev/fb1"

static int g_listen_fd = -1;
static int g_client_fd = -1;
static int g_dev_fd = -1;

static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static unsigned int screen_size = 0;
static unsigned char *screen = NULL;


static int do_send(int fd, char *buf, long int len)
{
    int send_len = 0;
    int tmp_len;

retry:
    while (send_len < len &&
           (tmp_len = send(fd, buf + send_len, len - send_len, 0)) > 0)
        send_len += tmp_len;

    if (tmp_len < 0 && (errno == EINTR)) {
        goto retry;
    } else if (tmp_len < 0) {
        printf("send() failed! errno=%d\n", errno);
    }

    printf("send_len=%d\n", send_len);
    return send_len;
}

static void main_loop(void)
{
    int addr_len = 0;
    struct sockaddr_in c_addr;
    char head[4] = {0};

    *(unsigned int *)head = htonl(screen_size);
    do {
        if ((g_client_fd = accept(g_listen_fd, (struct sockaddr *)&c_addr, &addr_len)) < 0) {
            printf("accept() failed! errno=%d", errno);
            sleep(1);
            continue;
        }
        printf("client_fd=%d, listen_fd=%d\n", g_client_fd, g_listen_fd);

        // send length firstly
        do_send(g_client_fd, head, sizeof(head));
        do_send(g_client_fd, screen, screen_size);

    } while (1);
}

int main(int argc, const char *argv[])
{
    struct sockaddr_in addr;

    int resock = 0;
    struct linger r;
    r.l_onoff = 1;
    r.l_linger = 0;

    int read_len = 0;
    int tmp_len = 0;

    unsigned long i = 0;

    // open fb device
    if ((g_dev_fd = open(ABS_DEV_PATH, O_RDONLY)) < 0) {
        printf("open dev(%s) failed! errno=%d\n", ABS_DEV_PATH, errno);
        return 0;
    }

    // open listen socket
    if ((g_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket() failed! errno=%d\n", errno);
        return 0;
    }

    if (setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &resock, sizeof(resock)) < 0) {
        printf("SETSOCKOPT REUSEADDR failed, errno=%d\n", errno);
    }

    if (setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEPORT, &resock, sizeof(resock)) < 0) {
        printf("SETSOCKOPT REUSEPORT failed, errno=%d\n", errno);
    }
    setsockopt(g_listen_fd, SOL_SOCKET, SO_LINGER, &r, sizeof(r));

    // bind address
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(HOST);

    if (bind(g_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("bind address failed, errno=%d\n", errno);
        return 0;
    }

    // listen connection
    if (listen(g_listen_fd, MAX_LISTEN_FD) < 0) {
        printf("listen failed, errno=%d\n", errno);
        return 0;
    }

    // get dev info
    if (ioctl(g_dev_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        printf("ioctl() FBIOGET_FSCREENINFO failed! errno=%d\n", errno);
        return 0;
    }
    printf("smem_len:%d\n", finfo.smem_len);
    printf("line_length:%d\n", finfo.line_length);

    if (ioctl(g_dev_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        printf("ioctl() FBIOGET_VSCREENINFO failed! errno=%d\n", errno);
        return 0;
    }
    printf("bits_per_pixel:%d\n", vinfo.bits_per_pixel);
    printf("xres:%d\n", vinfo.xres);
    printf("yres:%d\n", vinfo.yres);
    printf("red_length:%d\n", vinfo.red.length);
    printf("green_length:%d\n", vinfo.green.length);
    printf("blue_length:%d\n", vinfo.blue.length);
    printf("transp_length:%d\n", vinfo.transp.length);
    printf("red_offset:%d\n", vinfo.red.offset);
    printf("green_offset:%d\n", vinfo.green.offset);
    printf("blue_offset:%d\n", vinfo.blue.offset);
    printf("transp_offset:%d\n", vinfo.transp.offset);
    printf("xoffset:%d\n", vinfo.xoffset);
    printf("yoffset:%d\n", vinfo.yoffset);

    screen_size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    printf("screen_size=%d\n", screen_size);

#if 0
    // mmap dev
    if (!(screen = mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_dev_fd, 0))) {
        printf("mmap() failed! errno=%d\n", errno);
        return 0;
    }
#else
    if (!(screen = malloc(screen_size))) {
        printf("malloc(size=%d) failed!\n", screen_size);
        return 0;
    }
    memset(screen, 0, screen_size);

    while (read_len < screen_size &&
           (tmp_len = read(g_dev_fd, screen + read_len, screen_size - read_len)) > 0)
        read_len += tmp_len;

    if (read_len < screen_size) {
        printf("read() failed! errno=%d\n", errno);
        return 0;
    }

#endif

    main_loop();
}










