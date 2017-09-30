#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <giblib/giblib.h>
#include <X11/Xlib.h>

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

int main(void)
{
    struct screen screen;
    Imlib_Image image;

    screen_init(&screen);
    image = gib_imlib_create_image_from_drawable(screen.root, 0, 0, 0, screen.screen->width, screen.screen->height, 1);
    imlib_context_set_image(image);
    imlib_image_set_format("jpeg");
    imlib_save_image("screen.jpeg");
    imlib_free_image();
    exit(0);
}






