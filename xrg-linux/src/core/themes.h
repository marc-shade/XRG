#ifndef XRG_THEMES_H
#define XRG_THEMES_H

#include <gtk/gtk.h>

/**
 * Predefined color themes for XRG-Linux
 */

typedef struct {
    const char *name;
    GdkRGBA background_color;
    GdkRGBA graph_bg_color;
    GdkRGBA graph_fg1_color;
    GdkRGBA graph_fg2_color;
    GdkRGBA graph_fg3_color;
    GdkRGBA text_color;
    GdkRGBA border_color;
} XRGTheme;

/* Helper macro for creating RGB values */
#define RGB(r, g, b) {r/255.0, g/255.0, b/255.0, 1.0}
#define RGBA(r, g, b, a) {r/255.0, g/255.0, b/255.0, a}

/* Theme definitions */
static const XRGTheme XRG_THEMES[] = {
    /* Cyberpunk (default) - Electric cyan, magenta, green on dark blue */
    {
        .name = "Cyberpunk",
        .background_color = RGBA(5, 13, 31, 0.95),    /* Dark blue-black */
        .graph_bg_color = RGBA(13, 20, 38, 0.95),     /* Dark blue */
        .graph_fg1_color = RGB(0, 242, 255),          /* Electric Cyan */
        .graph_fg2_color = RGB(255, 0, 204),          /* Magenta */
        .graph_fg3_color = RGB(51, 255, 77),          /* Electric Green */
        .text_color = RGB(230, 255, 255),             /* Bright Cyan-White */
        .border_color = RGBA(0, 179, 230, 0.5)        /* Cyan border */
    },

    /* Nord - Cool pastel blues and teals */
    {
        .name = "Nord",
        .background_color = RGBA(46, 52, 64, 0.95),   /* Nord0 - Polar Night */
        .graph_bg_color = RGBA(59, 66, 82, 0.95),     /* Nord1 */
        .graph_fg1_color = RGB(136, 192, 208),        /* Nord8 - Frost */
        .graph_fg2_color = RGB(129, 161, 193),        /* Nord9 */
        .graph_fg3_color = RGB(163, 190, 140),        /* Nord14 - Aurora */
        .text_color = RGB(236, 239, 244),             /* Nord6 - Snow Storm */
        .border_color = RGBA(94, 129, 172, 0.5)       /* Nord10 */
    },

    /* Dracula - Purple and pink */
    {
        .name = "Dracula",
        .background_color = RGBA(40, 42, 54, 0.95),   /* Background */
        .graph_bg_color = RGBA(68, 71, 90, 0.95),     /* Current Line */
        .graph_fg1_color = RGB(139, 233, 253),        /* Cyan */
        .graph_fg2_color = RGB(255, 121, 198),        /* Pink */
        .graph_fg3_color = RGB(80, 250, 123),         /* Green */
        .text_color = RGB(248, 248, 242),             /* Foreground */
        .border_color = RGBA(189, 147, 249, 0.5)      /* Purple */
    },

    /* Gruvbox Dark - Warm retro colors */
    {
        .name = "Gruvbox",
        .background_color = RGBA(40, 40, 40, 0.95),   /* bg0_h */
        .graph_bg_color = RGBA(60, 56, 54, 0.95),     /* bg1 */
        .graph_fg1_color = RGB(131, 165, 152),        /* aqua */
        .graph_fg2_color = RGB(211, 134, 155),        /* purple */
        .graph_fg3_color = RGB(184, 187, 38),         /* yellow */
        .text_color = RGB(235, 219, 178),             /* fg0 */
        .border_color = RGBA(168, 153, 132, 0.5)      /* fg4 */
    },

    /* Solarized Dark - Classic low-contrast */
    {
        .name = "Solarized Dark",
        .background_color = RGBA(0, 43, 54, 0.95),    /* base03 */
        .graph_bg_color = RGBA(7, 54, 66, 0.95),      /* base02 */
        .graph_fg1_color = RGB(42, 161, 152),         /* cyan */
        .graph_fg2_color = RGB(108, 113, 196),        /* violet */
        .graph_fg3_color = RGB(181, 137, 0),          /* yellow */
        .text_color = RGB(131, 148, 150),             /* base0 */
        .border_color = RGBA(88, 110, 117, 0.5)       /* base01 */
    },

    /* Tokyo Night - Deep purples and blues */
    {
        .name = "Tokyo Night",
        .background_color = RGBA(26, 27, 38, 0.95),   /* bg */
        .graph_bg_color = RGBA(36, 40, 59, 0.95),     /* bg_dark */
        .graph_fg1_color = RGB(125, 207, 255),        /* blue */
        .graph_fg2_color = RGB(187, 154, 247),        /* purple */
        .graph_fg3_color = RGB(158, 206, 106),        /* green */
        .text_color = RGB(192, 202, 245),             /* fg */
        .border_color = RGBA(122, 162, 247, 0.5)      /* blue0 */
    },

    /* Catppuccin Mocha - Soft pastels on dark */
    {
        .name = "Catppuccin",
        .background_color = RGBA(30, 30, 46, 0.95),   /* base */
        .graph_bg_color = RGBA(49, 50, 68, 0.95),     /* surface0 */
        .graph_fg1_color = RGB(137, 220, 235),        /* sky */
        .graph_fg2_color = RGB(203, 166, 247),        /* mauve */
        .graph_fg3_color = RGB(166, 227, 161),        /* green */
        .text_color = RGB(205, 214, 244),             /* text */
        .border_color = RGBA(137, 180, 250, 0.5)      /* blue */
    },

    /* Monokai - Retro terminal feel */
    {
        .name = "Monokai",
        .background_color = RGBA(39, 40, 34, 0.95),   /* background */
        .graph_bg_color = RGBA(73, 72, 62, 0.95),     /* select */
        .graph_fg1_color = RGB(102, 217, 239),        /* cyan */
        .graph_fg2_color = RGB(174, 129, 255),        /* purple */
        .graph_fg3_color = RGB(166, 226, 46),         /* green */
        .text_color = RGB(248, 248, 242),             /* foreground */
        .border_color = RGBA(249, 38, 114, 0.5)       /* pink */
    }
};

#define XRG_THEME_COUNT (sizeof(XRG_THEMES) / sizeof(XRG_THEMES[0]))

#endif /* XRG_THEMES_H */
