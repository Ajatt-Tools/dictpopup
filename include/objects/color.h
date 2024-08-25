#ifndef COLOR_H
#define COLOR_H

typedef struct Color {
    double red;
    double green;
    double blue;
} Color;

#define ANKI_RED                                                                                   \
    (Color) {                                                                                      \
        .red = 0.9490, .green = 0.4431, .blue = 0.4431                                             \
    }
#define ANKI_GREEN                                                                                 \
    (Color) {                                                                                      \
        .red = 0.1333, .green = 0.7725, .blue = 0.3686                                             \
    }
#define ANKI_BLUE                                                                                  \
    (Color) {                                                                                      \
        .red = 0.5765, .green = 0.7725, .blue = 0.9922                                             \
    }
#define ORANGE                                                                                     \
    (Color) {                                                                                      \
        .red = 1, .green = 0.5, .blue = 0                                                          \
    }
#define GREY                                                                                       \
    (Color) {                                                                                      \
        .red = 0.5, .green = 0.5, .blue = 0.5                                                      \
    }

#endif // COLOR_H
