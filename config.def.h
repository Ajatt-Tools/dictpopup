static const char *background_color = "#2E3440";
static const char *border_color = "#ececec";
static const char *font_color = "#ececec";
static const char *font_pattern = "Noto Sans Mono CJK JP:size=12";
static const unsigned line_spacing = 5;
static const unsigned int padding = 15;

static const unsigned int width = 480;
static const unsigned int border_size = 1;

#define MIN_BORDER_DISTANCE 3 /* The minimum distance to the monitor boundary */

#define DISMISS_BUTTON Button1 /* left click */
#define ACTION1_BUTTON Button2 /* middle click */
#define ACTION2_BUTTON Button5 /* scroll down */

/* exit codes */
#define EXIT_DISMISS 0
#define EXIT_FAIL 1
#define EXIT_ACTION1 2
#define EXIT_ACTION2 3
