/*
 * conge.h - a console graphics engine.
 */

#ifndef _CONGE_H
#define _CONGE_H

#include <stdlib.h>
#include <stdio.h> /* sprintf is useful for conge_write_string */
#include <string.h>
#include <io.h>
#include <sys/timeb.h>

/* Make sure the math constants are defined. */
#define _USE_MATH_DEFINES
#include <math.h>

#define _WIN32_WINNT 0x0500
#include <windows.h>

/* Weird stuff. */
#undef near
#undef far

/* How many scancodes are supported. */
#define CONGE_SCANCODE_COUNT 256

#define CONGE_MIN(A, B) ((A) < (B) ? (A) : (B))
#define CONGE_MAX(A, B) ((A) > (B) ? (A) : (B))

typedef unsigned char conge_color;

typedef char conge_bool;

typedef struct conge_pixel conge_pixel;
struct conge_pixel
{
  unsigned char character;
  conge_color fg, bg; /* character and background color */
};

typedef struct conge_ctx conge_ctx;
struct conge_ctx
{
  /* Public API. Read-only unless specified otherwise. */
  conge_pixel* frame; /* output: the frame being rendered */
  int rows, cols; /* window size in characters */
  double delta; /* previous frame's delta time */
  double timestep; /* the fixed timestep for specified FPS */
  conge_bool keys[CONGE_SCANCODE_COUNT]; /* if true, that key is down */
  int scroll; /* forward if 1, backward if -1, and no scrolling if 0 */
  char buttons; /* a mouse button is held if its respective flag is set */
  int mouse_x, mouse_y; /* the character the mouse is hovering over */
  int mouse_dx, mouse_dy; /* mouse position relative to the previous frame */
  conge_bool grab; /* output: set this to grab/ungrab the mouse */
  conge_bool exit; /* output: when set to true, the program will exit */
  int fps; /* the current FPS */
  char title[128]; /* output: the console window title */

  /* Internal API. Avoid at all cost! */
  struct
  {
    HANDLE input, output; /* console IO handles */
    HWND c_window; /* console window handle */
    conge_pixel* backbuffer; /* double-buffering support */
    int cursor_x, cursor_y; /* prevent unnecessary cursor movements */
    int last_color; /* prevent changing the color for every pixel */
  } internal;
};

/* The function called before rendering each frame. */
typedef void (*conge_callback) (conge_ctx* ctx);

/* TODO: add mouse wheel click. */
#define CONGE_LMB FROM_LEFT_1ST_BUTTON_PRESSED
#define CONGE_RMB RIGHTMOST_BUTTON_PRESSED

/*
 * Initialize a new conge context. Return NULL if malloc failed.
 */
conge_ctx* conge_init (void);

/*
 * Run the conge mainloop with a maximum FPS, calling TICK every frame.
 *
 * Return codes:
 *   0 on success.
 *   1 if CTX hasn't been initialized.
 *   2 if TICK is null.
 *   3 if max_fps is negative or zero.
 *   4 if a memory error has occured.
 */
int conge_run (conge_ctx*, conge_callback, int);

/*
 * Free a conge_ctx object. Do nothing if it's already null.
 */
void conge_free (conge_ctx*);

/*
 * Return the pixel at specified position from the current frame.
 *
 * (0; 0) is the top-left corner of the screen. The pixel can be altered.
 *
 * Only use inside the tick callback!
 *
 * Return null if X or Y are out of screen bounds, or CTX is null.
 */
conge_pixel* conge_get_pixel (conge_ctx*, int, int);

/*
 * Fill the specified screen position with PIXEL.
 * Skip if X or Y are out of bounds.
 *
 * Return codes:
 *   0 on success.
 *   1 if CTX is null.
 */
int conge_fill (conge_ctx*, int, int, conge_pixel);

/*
 * Draw a line between two points, filled with the specified pixel.
 *
 * Return codes:
 *   0 on success.
 *   1 if CTX is null.
 */
int conge_draw_line (conge_ctx*, int, int, int, int, conge_pixel);

/*
 * Draw a triangle defined by three of its
 * vertices in _counter-clockwise_ order.
 *
 * Return codes:
 *   0 on success.
 *   1 if CTX is null.
 */
int conge_draw_triangle (conge_ctx*, int, int, int, int,
                         int, int, conge_pixel);
/*
 * Write a string with specified position and color onto the frame.
 *
 * If it doesn't fit, it gets cut off.
 *
 * Return codes:
 *   0 on success.
 *   1 if CTX is null.
 *   2 if STRING is null.
 */
int conge_write_string (conge_ctx*, char*, int, int, conge_color, conge_color);

/*
 * Internal: draw the current frame.
 */
void conge_draw_frame (conge_ctx*);

/*
 * Internal: handle input for the frame.
 */
void conge_handle_input (conge_ctx*);

/* Color names. */
enum
  {
    CONGE_BLACK,
    CONGE_BLUE,
    CONGE_GREEN,
    CONGE_AQUA,
    CONGE_RED,
    CONGE_PURPLE,
    CONGE_YELLOW,
    CONGE_WHITE,
    CONGE_GRAY,
    CONGE_LBLUE,
    CONGE_LGREEN,
    CONGE_LAQUA,
    CONGE_LRED,
    CONGE_LPURPLE,
    CONGE_LYELLOW,
    CONGE_BWHITE,
  };

/* Scancodes named after their US keycaps. */
enum
  {
    CONGE_EMPTY,
    CONGE_ESC,
    CONGE_1,
    CONGE_2,
    CONGE_3,
    CONGE_4,
    CONGE_5,
    CONGE_6,
    CONGE_7,
    CONGE_8,
    CONGE_9,
    CONGE_0,
    CONGE_HYPHEN,
    CONGE_EQUALS,
    CONGE_BACKSPACE,
    CONGE_TAB,
    CONGE_Q,
    CONGE_W,
    CONGE_E,
    CONGE_R,
    CONGE_T,
    CONGE_Y,
    CONGE_U,
    CONGE_I,
    CONGE_O,
    CONGE_P,
    CONGE_LEFT_BRACKET,
    CONGE_RIGHT_BRACKET,
    CONGE_ENTER,
    CONGE_LCTRL,
    CONGE_A,
    CONGE_S,
    CONGE_D,
    CONGE_F,
    CONGE_G,
    CONGE_H,
    CONGE_J,
    CONGE_K,
    CONGE_L,
    CONGE_SEMICOLON,
    CONGE_QUOTE,
    CONGE_GRAVE, /* also known as tilde */
    CONGE_LSHIFT,
    CONGE_BACKSLASH,
    CONGE_Z,
    CONGE_X,
    CONGE_C,
    CONGE_V,
    CONGE_B,
    CONGE_N,
    CONGE_M,
    CONGE_COMMA,
    CONGE_FULL_STOP,
    CONGE_SLASH,
    CONGE_RSHIFT,
    CONGE_PRT_SCR,
    CONGE_LALT,
    CONGE_SPACEBAR,
    CONGE_CAPS_LOCK,
    CONGE_F1,
    CONGE_F2,
    CONGE_F3,
    CONGE_F4,
    CONGE_F5,
    CONGE_F6,
    CONGE_F7,
    CONGE_F8,
    CONGE_F9,
    CONGE_F10,
    CONGE_NUMLOCK,
    CONGE_SCROLL_LOCK,
    CONGE_KP_7,
    CONGE_KP_8,
    CONGE_KP_9,
    CONGE_KP_MINUS,
    CONGE_KP_4,
    CONGE_KP_5,
    CONGE_KP_6,
    CONGE_KP_PLUS,
    CONGE_KP_1,
    CONGE_KP_2,
    CONGE_KP_3,
    CONGE_KP_0,
    CONGE_KP_DOT,
  };

/* The empty, "clear" pixel. */
static conge_pixel conge_empty = {' ', CONGE_WHITE, CONGE_BLACK};

#endif /* _CONGE_H */
