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

/* Weird stuff from the evil header above. */
#undef near
#undef far

#define CONGE_MIN(A, B) ((A) < (B) ? (A) : (B))
#define CONGE_MAX(A, B) ((A) > (B) ? (A) : (B))

/* An ASCII character and two 16-color variables can fit into two bytes. */
typedef unsigned short int conge_pixel;

/* Internal constant. 256 scancodes divided by sizeof (int) in bits. */
#define CONGE__KEYS_LENGTH (32 / sizeof (int))

/* The ConGE context, which is required to run the engine. */
typedef struct conge_ctx conge_ctx;
struct conge_ctx
{
  /* Public API. Read-only unless specified otherwise. */
  conge_pixel* frame; /* output: the frame being rendered */
  int rows, cols; /* window size in characters */
  double delta; /* previous frame's delta time */
  double elapsed; /* seconds since the engine was started */
  double timestep; /* the fixed timestep for specified FPS */
  int scroll; /* forward if 1, backward if -1, and no scrolling if 0 */
  int mouse_x, mouse_y; /* the character the mouse is hovering over */
  int mouse_dx, mouse_dy; /* mouse position relative to the previous frame */
  int grab; /* output: set this to grab/ungrab the mouse */
  int exit; /* output: when set to true, the program will exit */
  double fps; /* the current FPS */
  unsigned int ticks; /* the total amount of ticks done */
  char title[128]; /* output: the console window title */
  /* Internal API; avoid at all cost! */
  HANDLE _input, _output; /* console IO handles */
  HWND _window; /* console window handle */
  conge_pixel* _backbuffer; /* double-buffering support */
  int _keys[CONGE__KEYS_LENGTH]; /* a 256-bit bitflag */
  int _prev_keys[CONGE__KEYS_LENGTH]; /* handle "just pressed" events */
  int _buttons; /* the currently held mouse buttons */
  int _cursor_x, _cursor_y; /* prevent unnecessary cursor movements */
  int _last_color; /* same for changing the color */
};

/* The function called before rendering each frame. */
typedef void (*conge_tick) (conge_ctx* ctx);

/* TODO: add mouse wheel click. */
#define CONGE_LMB FROM_LEFT_1ST_BUTTON_PRESSED
#define CONGE_RMB RIGHTMOST_BUTTON_PRESSED

/*
 * Initialize a new ConGE context. Return NULL if memory allocation failed.
 */
conge_ctx* conge_init ();

/*
 * Run the ConGE mainloop.
 *
 * TICK must be a function which takes CTX as its only argument. It will be
 * called at most MAX_FPS times per second.
 *
 * Return codes:
 *   0 - TICK requested exit (by setting CTX->exit to true).
 *   1 - CTX is null.
 *   2 - MAX_FPS is negative or zero.
 *   3 - failed to allocate one of the screen buffers.
 */
int conge_run (conge_ctx* ctx, conge_tick tick, int max_fps);

/*
 * Free the allocated ConGE context.
 */
void conge_free (conge_ctx*);

/*
 * Create a new pixel from a character and its bg and fg colors.
 */
conge_pixel conge_new_pixel (unsigned char, int fg, int bg);

unsigned char conge_get_character (conge_pixel);
void conge_set_character (conge_pixel*, char);

int conge_get_fg (conge_pixel);
void conge_set_fg (conge_pixel*, int);

int conge_get_bg (conge_pixel);
void conge_set_bg (conge_pixel*, int);

/*
 * Return the pixel at specified position from the current frame.
 *
 * (0; 0) is the top-left corner of the screen. The pixel can be modified.
 *
 * Only use inside the tick callback!
 *
 * Return null if X or Y are out of screen bounds, or CTX is null.
 */
conge_pixel* conge_get_pixel (conge_ctx*, int x, int y);

/*
 * Return 1 if the given key, identified by its scancode, is held down.
 *
 * Return 0 otherwise or if CTX is null.
 */
int conge_is_key_down (conge_ctx*, int code);

/*
 * Return 1 if the given key was just pressed.
 *
 * Return 0 otherwise or if CTX is null.
 */
int conge_is_key_just_pressed (conge_ctx*, int code);

/*
 * Return 1 if the given mouse button (CONGE_LMB or CONGE_RMB) is down.
 *
 * Return 0 otherwise or if CTX is null.
 */
int conge_is_button_down (conge_ctx*, int mask);

/*
 * Fill the specified screen position with PIXEL.
 * Skip if X or Y are out of bounds.
 *
 * Return codes:
 *   0 - success.
 *   1 - CTX is null.
 */
int conge_fill (conge_ctx*, int x, int y, conge_pixel);

/*
 * Draw a line between two points, filled with the specified pixel.
 *
 * Return codes:
 *   0 - success.
 *   1 - CTX is null.
 */
int conge_draw_line (conge_ctx*, int x0, int y0, int x1, int y1, conge_pixel);

/*
 * Fill a triangle, defined by the three of its vertices.
 *
 * The vertices must come in counter-clockwise order.
 *
 * Return codes:
 *   0 - success.
 *   1 - CTX is null.
 */
int conge_fill_triangle (conge_ctx*, int, int, int, int,
                         int, int, conge_pixel);
/*
 * Write a string with specified position and color onto the frame.
 *
 * If it doesn't fit, the string gets cut off.
 *
 * Return codes:
 *   0 - success.
 *   1 - CTX is null.
 *   2 - STRING is null.
 */
int conge_write_string (conge_ctx*, const char*, int, int, int fg, int bg);

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
    CONGE_BRIGHT_BLUE,
    CONGE_BRIGHT_GREEN,
    CONGE_BRIGHT_AQUA,
    CONGE_BRIGHT_RED,
    CONGE_BRIGHT_PURPLE,
    CONGE_BRIGHT_YELLOW,
    CONGE_BRIGHT_WHITE,
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

#endif /* _CONGE_H */
