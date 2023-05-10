#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include "config.h"

/* macros */
#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define INTERSECT(x,y,w,h,r)  (MAX(0, MIN((x)+(w),(r).x_org+(r).width)  - MAX((x),(r).x_org)) \
                             * MAX(0, MIN((y)+(h),(r).y_org+(r).height) - MAX((y),(r).y_org)))

#define EXIT_ACTION 0
#define EXIT_FAIL 1
#define EXIT_DISMISS 2

static Display *dpy;
static Window window;
static int mon = -1, screen;
static int mw, mh;

int exit_code = EXIT_DISMISS;

static void die(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(EXIT_FAIL);
}

int get_max_len(char *string, XftFont *font, int max_text_width)
{
	int eol = strlen(string);
	XGlyphInfo info;
	XftTextExtentsUtf8(dpy, font, (FcChar8 *)string, eol, &info);

	if (info.width > max_text_width)
	{
		eol = max_text_width / font->max_advance_width;
		info.width = 0;

		while (info.width < max_text_width)
		{
			eol++;
			XftTextExtentsUtf8(dpy, font, (FcChar8 *)string, eol, &info);
		}

		eol--;
	}

	for (int i = 0; i < eol; i++)
		if (string[i] == '\n')
		{
			string[i] = ' ';
			return ++i;
		}

	//Don't split in the middle of unicode char
	while (eol && (string[eol] & 0xC0) == 0x80)
		--eol;

	if (info.width <= max_text_width)
		return eol;

	int temp = eol;

	while (string[eol] != ' ' && eol)
		--eol;

	if (eol == 0)
		return temp;
	else
		return ++eol;
}

void expire(int sig)
{
	XEvent event;
	event.type = ButtonPress;
	event.xbutton.button = (sig == SIGUSR2) ? (ACTION_BUTTON) : (DISMISS_BUTTON);
	XSendEvent(dpy, window, 0, 0, &event);
	XFlush(dpy);
}

int main(int argc, char *argv[])
{
	if (argc == 1)
		die("Usage: %s body", argv[0]);

	struct sigaction act_expire, act_ignore;

	act_expire.sa_handler = expire;
	act_expire.sa_flags = SA_RESTART;
	sigemptyset(&act_expire.sa_mask);

	act_ignore.sa_handler = SIG_IGN;
	act_ignore.sa_flags = 0;
	sigemptyset(&act_ignore.sa_mask);

	sigaction(SIGALRM, &act_expire, 0);
	sigaction(SIGTERM, &act_expire, 0);
	sigaction(SIGINT, &act_expire, 0);

	sigaction(SIGUSR1, &act_ignore, 0);
	sigaction(SIGUSR2, &act_ignore, 0);

	if (!(dpy = XOpenDisplay(0)))
		die("Cannot open display");

	screen = DefaultScreen(dpy);
	window = RootWindow(dpy, screen);
	Visual *visual = DefaultVisual(dpy, screen);
	Colormap colormap = DefaultColormap(dpy, screen);

	XSetWindowAttributes attributes;
	attributes.override_redirect = True;
	XftColor color;
	XftColorAllocName(dpy, visual, colormap, background_color, &color);
	attributes.background_pixel = color.pixel;
	XftColorAllocName(dpy, visual, colormap, border_color, &color);
	attributes.border_pixel = color.pixel;

	int num_of_lines = 0;
	int max_text_width = width - 2 * padding;
	int lines_size = 5;
	char **lines = malloc(lines_size * sizeof(char *));
	if (!lines)
		die("malloc failed");

	XftFont *font = XftFontOpenName(dpy, screen, font_pattern);

	for (int i = 1; i < argc; i++)
	{
		for (unsigned int eol = get_max_len(argv[i], font, max_text_width); eol; argv[i] += eol, num_of_lines++, eol = get_max_len(argv[i], font, max_text_width))
		{
			if (lines_size <= num_of_lines)
			{
				lines = realloc(lines, (lines_size += 5) * sizeof(char *));
				if (!lines)
					die("realloc failed");
			}

			lines[num_of_lines] = malloc((eol + 1) * sizeof(char));
			if (!lines[num_of_lines])
				die("malloc failed");

			strncpy(lines[num_of_lines], argv[i], eol);
			lines[num_of_lines][eol] = '\0';
		}
	}

	unsigned int text_height = font->ascent - font->descent;
	unsigned int height = (num_of_lines - 1) * line_spacing + num_of_lines * text_height + 2 * padding;

	int x, y, di;
	unsigned int du;
	Window dw;
	if (!XQueryPointer(dpy, window, &dw, &dw, &x, &y, &di, &di, &du))
	  die("Could not query pointer position");
#ifdef XINERAMA
	XineramaScreenInfo *info;
	int n, i=0;

	// TODO: Better error handling like dmenu
	if ((info = XineramaQueryScreens(dpy, &n))){
	  if (mon < 0)
		  for (i = 0; i < n; i++)
			  if (INTERSECT(x, y, 1, 1, info[i]) != 0)
				  break;

	  int x_offset = info[i].x_org;
	  int y_offset = info[i].y_org;
	  mh = info[i].height;
	  mw = info[i].width;
	  XFree(info);

	  // Correct on monitor boundary
	  if (y - y_offset + height > mh)
	    y -= height;
	  if (x - x_offset + width > mw)
	    x -= width;
	}
#endif

	window = XCreateWindow(dpy, RootWindow(dpy, screen), x, y, width, height, border_size, DefaultDepth(dpy, screen),
						   CopyFromParent, visual, CWOverrideRedirect | CWBackPixel | CWBorderPixel, &attributes);

	XftDraw *draw = XftDrawCreate(dpy, window, visual, colormap);
	XftColorAllocName(dpy, visual, colormap, font_color, &color);

	XSelectInput(dpy, window, ExposureMask | ButtonPress);
	XMapWindow(dpy, window);

	sigaction(SIGUSR1, &act_expire, 0);
	sigaction(SIGUSR2, &act_expire, 0);

	for (;;)
	{
		XEvent event;
		XNextEvent(dpy, &event);

		if (event.type == Expose)
		{
			XClearWindow(dpy, window);
			for (int i = 0; i < num_of_lines; i++)
				XftDrawStringUtf8(draw, &color, font, padding, line_spacing * i + text_height * (i + 1) + padding,
								  (FcChar8 *)lines[i], strlen(lines[i]));
		}
		/* else if (event.type == ButtonPress) */
		else if (event.type == ButtonPress)
		{
			if (event.xbutton.button == DISMISS_BUTTON)
				break;
			else if (event.xbutton.button == ACTION_BUTTON)
			{
				exit_code = EXIT_ACTION;
				break;
			}
		}
	}

	for (int i = 0; i < num_of_lines; i++)
		free(lines[i]);

	free(lines);
	XftDrawDestroy(draw);
	XftColorFree(dpy, visual, colormap, &color);
	XftFontClose(dpy, font);
	XCloseDisplay(dpy);

	return exit_code;
}