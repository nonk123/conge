/*
 * conge.h - a console graphics engine.
 */

#include <stdlib.h>
#include <stdio.h> /* sprintf is useful for conge_write_string */
#include <string.h>
#include <io.h>
#include <math.h>
#include <sys/timeb.h>

#include <windows.h>

/* How many scancodes are supported. */
#define CONGE_SCANCODE_COUNT 256

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
  int scroll; /* forward if 1, backward if -1, and no scrolling if 0. */
  char buttons; /* the mouse button is held if its respective flag is set */
  int mouse_x, mouse_y; /* character the position the mouse is hovering over */
  conge_bool exit; /* output: when set to true, the program will exit */
  int fps; /* the current FPS */
  char title[128]; /* output: the console window title */

  /* Internal API. Avoid at all cost! */
  struct
  {
    HANDLE input, output; /* console IO handles */
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

enum conge_color_names
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

/* Named after their US-layout symbols. */
enum conge_scancode_names
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
    /* TODO: add more. */
  };

/* The empty, "clear" pixel. */
static conge_pixel conge_empty = {' ', CONGE_WHITE, CONGE_BLACK};

/*
 * Initialize a new conge context. Return NULL if malloc failed.
 */
conge_ctx*
conge_init (void)
{
  int i;

  conge_ctx* ctx = malloc (sizeof *ctx);

  if (ctx == NULL)
    return NULL;

  ctx->exit = 0;
  ctx->fps = 0;

  ctx->scroll = 0;
  ctx->buttons = 0;

  ctx->mouse_x = 0;
  ctx->mouse_y = 0;

  for (i = 0; i < CONGE_SCANCODE_COUNT; i++)
    ctx->keys[i] = 0;

  strcpy (ctx->title, "conge");

  ctx->frame = NULL;

  ctx->internal.input = GetStdHandle (STD_INPUT_HANDLE);
  ctx->internal.output = GetStdHandle (STD_OUTPUT_HANDLE);

  ctx->internal.cursor_x = 0;
  ctx->internal.cursor_y = 0;

  ctx->internal.last_color = 0;

  ctx->internal.backbuffer = NULL;

  return ctx;
}

/*
 * Free a conge_ctx object.
 */
void
conge_free (conge_ctx* ctx)
{
  if (ctx != NULL)
    free (ctx);
}

/*
 * Return the pixel at specified position from the current frame.
 *
 * (0; 0) is the top-left corner of the screen. The pixel can be altered.
 *
 * Only use inside the tick callback!
 *
 * If the specified CTX is null, return null also.
 */
conge_pixel*
conge_get_pixel (conge_ctx* ctx, int x, int y)
{
  if (ctx == NULL)
    return NULL;
  else
    return &ctx->frame[ctx->rows * x + y];
}

/*
 * Write a string with specified position and color onto the frame.
 *
 * If it doesn't fit, it gets cut off.
 *
 * Return codes:
 *   1 if CTX is null.
 *   2 if STRING is null.
 */
int
conge_write_string (conge_ctx* ctx, char* string, int x, int y, conge_color fg, conge_color bg)
{
  int i, len;

  if (ctx == NULL)
    return 1;

  if (string == NULL)
    return 2;

  len = strlen (string);

  for (i = 0; i < len; i++)
    {
      int char_x = x + i;

      if (char_x >= ctx->cols)
        break;
      else
        {
          conge_pixel* pixel = conge_get_pixel (ctx, char_x, y);

          pixel->character = string[i];
          pixel->fg = fg;
          pixel->bg = bg;
        }
    }
}

/*
 * For internal use: disable the display of the console cursor.
 */
void
conge_disable_cursor (conge_ctx* ctx)
{
  CONSOLE_CURSOR_INFO info;

  info.dwSize = 100;
  info.bVisible = FALSE;

  SetConsoleCursorInfo (ctx->internal.output, &info);
}

/*
 * Used internally to get the console window's size.
 */
void
conge_get_window_size (conge_ctx* ctx)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo (ctx->internal.output, &csbi);

  ctx->cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  ctx->rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

/*
 * For internal use: move the cursor unless putchar could do it.
 */
void
conge_move_cursor_to (conge_ctx* ctx, int x, int y)
{
  /* Writing to stdout moves the cursor right anyways. */
  if (ctx->internal.cursor_y != y || ctx->internal.cursor_x != x - 1)
    {
      COORD coord;

      coord.X = x;
      coord.Y = y;

      SetConsoleCursorPosition (ctx->internal.output, coord);
    }

  ctx->internal.cursor_x = x;
  ctx->internal.cursor_y = y;
}

/*
 * For internal use: print all subsequent characters in this color.
 */
void
conge_set_text_color (conge_ctx* ctx, int color)
{
  if (ctx->internal.last_color != color)
    {
      SetConsoleTextAttribute (ctx->internal.output, color);
      ctx->internal.last_color = color;
    }
}

/*
 * Internal: draw the current frame with double-buffering.
 */
void
conge_draw_frame (conge_ctx* ctx)
{
  int x, y;

  /* Kludge: trigger the initial cursor movement. */
  ctx->internal.cursor_x = -2;
  ctx->internal.cursor_y = 0;

  for (y = 0; y < ctx->rows; y++)
    for (x = 0; x < ctx->cols; x++)
      {
        conge_pixel* front = conge_get_pixel (ctx, x, y);
        conge_pixel* back = &ctx->internal.backbuffer[ctx->rows * x + y];

        /* If front and back pixels differ, print the front one. */
        if (front->character != back->character
            || front->fg != back->fg || front->bg != back->bg)
          {
            conge_move_cursor_to (ctx, x, y);
            conge_set_text_color (ctx, (16 * front->bg) + front->fg);
            _write (1, &front->character, 1);

            *back = *front;
          }
      }

  /* Prevent visual glitches. */
  conge_move_cursor_to (ctx, 0, 0);
}

/*
 * Internal: populate the input-related variables.
 */
void
conge_handle_input (conge_ctx* ctx)
{
  int i;

  INPUT_RECORD records[10];
  DWORD count;

  ctx->scroll = 0;

  GetNumberOfConsoleInputEvents (ctx->internal.input, &count);

  if (!count)
    return;

  ReadConsoleInput (ctx->internal.input, records, 10, &count);

  for (i = 0; i < count; i++)
    {
      if (records[i].EventType == KEY_EVENT)
        {
          KEY_EVENT_RECORD event = records[i].Event.KeyEvent;
          WORD scancode = event.wVirtualScanCode;

          if (scancode < CONGE_SCANCODE_COUNT)
            ctx->keys[scancode] = event.bKeyDown;
        }
      else if (records[i].EventType == MOUSE_EVENT)
        {
          MOUSE_EVENT_RECORD event = records[i].Event.MouseEvent;

          ctx->mouse_x = event.dwMousePosition.X;
          ctx->mouse_y = event.dwMousePosition.Y;

          if (event.dwEventFlags & MOUSE_WHEELED)
            {
              int scroll = event.dwButtonState;
              ctx->scroll = scroll / abs (scroll);
            }

          ctx->buttons = event.dwButtonState;
        }
    }
}

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
int
conge_run (conge_ctx* ctx, conge_callback tick, int max_fps)
{
  struct timeb start, end; /* used for measuring delta */

  /* FPS measurement. */
  int frames_count = 0;
  double second = 0.0;

  int screen_area = 0; /* used to detect changes in resolution */
  int buffer_size, prev_buffer_size = 0;

  int exit = 0;

  if (ctx == NULL)
    return 1;

  if (tick == NULL)
    return 2;

  if (max_fps < 1)
    return 3;

  ctx->timestep = 1.0 / max_fps;

  ctx->frame = NULL;
  ctx->internal.backbuffer = NULL;

  /* Enable mouse support. */
  DWORD mouse_flags = ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
  SetConsoleMode (ctx->internal.input, mouse_flags);

  while (!ctx->exit)
    {
      int i;

      ftime (&start);

      conge_get_window_size (ctx);

      screen_area = ctx->rows * ctx->cols;
      buffer_size = screen_area * sizeof (conge_pixel);

      ctx->frame = realloc (ctx->frame, buffer_size);
      ctx->internal.backbuffer = realloc (ctx->internal.backbuffer, buffer_size);

      if (ctx->frame == NULL || ctx->internal.backbuffer == NULL)
        {
          exit = 4;
          break;
        }

      /* Clear the screen. */
      for (i = 0; i < screen_area; i++)
        ctx->frame[i] = conge_empty;

      /* Force a redraw when the window size changes. */
      if (prev_buffer_size != buffer_size)
        {
          conge_disable_cursor (ctx); /* the cursor reactivates after a resize */
          memset (ctx->internal.backbuffer, 0, buffer_size); /* fill with junk */
          prev_buffer_size = buffer_size;
        }

      conge_handle_input (ctx);
      tick (ctx);
      conge_draw_frame (ctx);

      SetConsoleTitle (ctx->title);

      ftime (&end);

      /* Measure the delta time in seconds. */
      ctx->delta = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);

      /* Sleep in order to prevent the game from running too quickly. */
      if (ctx->delta < ctx->timestep)
        {
          Sleep (1000 * (ctx->timestep - ctx->delta));
          ctx->delta = ctx->timestep;
        }

      second += ctx->delta;
      frames_count++;

      if (second >= 1.0)
        {
          ctx->fps = frames_count;
          frames_count = 0;
          second -= 1.0;
        }
    }

  if (ctx->frame != NULL)
    {
      free (ctx->frame);
      ctx->frame = NULL;
    }

  if (ctx->internal.backbuffer != NULL)
    {
      free (ctx->internal.backbuffer);
      ctx->internal.backbuffer = NULL;
    }

  /* Fix colors in the console before exiting. */
  conge_set_text_color (ctx, CONGE_WHITE);

  return exit;
}
