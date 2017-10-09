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
#include <sys/stat.h>
#include <linux/fb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <giblib/giblib.h>
#include <X11/Xlib.h>

#define HOST "192.168.31.58"
#define PORT 7465
#define MAX_LISTEN_FD 256

static int g_listen_fd = -1;
static int g_client_fd = -1;
static unsigned int screen_size = 0;
static char *screen_buf = NULL;

struct screen {
    Display *display;
    Screen *screen;
    Visual *visual;
    Colormap colormap;
    Window root;
};

void screen_init(struct screen *screen)
{
    screen->display = XOpenDisplay(NULL);
    if (!screen->display) {
        perror("XOpenDisplay");
        exit(0);
    }

    screen->screen = ScreenOfDisplay(screen->display, DefaultScreen(screen->display));
    screen->visual = DefaultVisual(screen->display, DefaultScreen(screen->display));
    screen->colormap = DefaultColormap(screen->display, DefaultScreen(screen->display));
    screen->root = RootWindow(screen->display, DefaultScreen(screen->display));

    imlib_context_set_display(screen->display);
    imlib_context_set_visual(screen->visual);
    imlib_context_set_colormap(screen->colormap);
}

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

    return send_len;
}

static void main_loop(void)
{
    unsigned int addr_len = 0;
    struct sockaddr_in c_addr;
    char head[4] = {0};

    struct screen screen;
    Imlib_Image image;
    struct stat fstat;
    FILE *fp = NULL;
    int read_len = 0;

    do {
        if ((g_client_fd = accept(g_listen_fd, (struct sockaddr *)&c_addr, &addr_len)) < 0) {
            sleep(1);
            continue;
        }

        screen_init(&screen);
        image = gib_imlib_create_image_from_drawable(screen.root, 0, 0, 0, screen.screen->width, screen.screen->height, 1);
        imlib_context_set_image(image);
        imlib_image_set_format("jpeg");
        imlib_save_image("/tmp/screen.jpeg");
        imlib_free_image();

        // send length firstly
        if (stat("/tmp/screen.jpeg", &fstat) < 0) {
            close(g_client_fd);
            continue;
        }

        screen_size = fstat.st_size;
        *(unsigned int *)head = htonl(screen_size);
        do_send(g_client_fd, head, sizeof(head));

        // send /tmp/screen.jpeg to client
        if (!(screen_buf = malloc(screen_size))) {
            close(g_client_fd);
            continue;
        }
        memset(screen_buf, 0, screen_size);

        if ((fp = fopen("/tmp/screen.jpeg", "rb")) < 0) {
            close(g_client_fd);
            free(screen_buf);
            continue;
        }

        if ((read_len = fread(screen_buf, screen_size, 1, fp)) <= 0) {
            close(g_client_fd);
            free(screen_buf);
            continue;
        }

        do_send(g_client_fd, screen_buf, screen_size);
        close(g_client_fd);
        unlink("/tmp/screen.jpeg");
        free(screen_buf);
    } while (1);
}

int main(int argc, const char *argv[])
{
    struct sockaddr_in addr;

    int resock = 0;
    struct linger r;
    r.l_onoff = 1;
    r.l_linger = 0;

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

    main_loop();
}










