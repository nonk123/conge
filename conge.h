#include <stdlib.h>
#include <io.h>
#include <math.h>
#include <sys/timeb.h>

#include <windows.h>

// clang-format off

typedef unsigned char conge_color;
typedef char conge_bool;

typedef struct conge_pixel {
  unsigned char character;
  conge_color fg, bg; /* character and background color */
} conge_pixel;

typedef conge_pixel* conge_frame;

/* How many scancodes  */
#define CONGE_SCANCODE_COUNT 256

typedef struct conge_ctx {
  /* Public API. */
  conge_frame frame; /* the frame being rendered */
  int rows, cols; /* read-only: window size in characters */
  double delta; /* read-only: previous frame's delta time */
  /* Access the elements with the scancode constants. */
  conge_bool keys[CONGE_SCANCODE_COUNT]; /* if true, the key is down */
  int scroll; /* forward if 1, backward if -1, and no scrolling if 0. */
  char buttons; /* the mouse button is held if its respective flag is set */
  int mouse_x, mouse_y; /* character the position the mouse is hovering over */
  conge_bool exit; /* when set to true, the program will exit */
  int fps; /* read-only: the current FPS */
  /* Internal API. Avoid at all cost! */
  struct {
    HANDLE input, output; /* console handles */
    conge_frame backbuffer; /* double-buffering support */
    int cursor_x, cursor_y; /* helps with tellling if cursor should be moved */
    int last_color; /* prevent changing the color for every pixel */
  } internal;
} conge_ctx;

/* The function called every tick before rendering. */
typedef void (*conge_callback)(conge_ctx* ctx);

enum conge_color_names {
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

/* No wheel click yet. */
#define CONGE_LMB FROM_LEFT_1ST_BUTTON_PRESSED
#define CONGE_RMB RIGHTMOST_BUTTON_PRESSED

/* Named after their US-layout symbols. */
enum conge_scancode_names {
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

static conge_ctx* conge_init()
{
  int i;

  conge_ctx* ctx = malloc(sizeof(conge_ctx));

  ctx->exit = 0;
  ctx->fps = 0;

  ctx->scroll = 0;
  ctx->buttons = 0;

  ctx->mouse_x = 0;
  ctx->mouse_y = 0;

  for (i = 0; i < CONGE_SCANCODE_COUNT; i++)
    ctx->keys[i] = 0;

  ctx->frame = NULL;

  ctx->internal.input = GetStdHandle(STD_INPUT_HANDLE);
  ctx->internal.output = GetStdHandle(STD_OUTPUT_HANDLE);

  ctx->internal.cursor_x = 9999;
  ctx->internal.cursor_y = 9999;

  ctx->internal.last_color = 0;

  ctx->internal.backbuffer = NULL;

  return ctx;
}

static void conge_free(conge_ctx* ctx)
{
  free(ctx);
}

/*
 * Return the pixel at specified position from the current frame.
 *
 * (0; 0) is the top-left corner of the screen. The pixel can be altered.
 *
 * Only use inside the tick callback!
 */
static conge_pixel* conge_get_pixel(conge_ctx* ctx, int x, int y)
{
  return &ctx->frame[ctx->rows * x + y];
}

/*
 * For internal use: disable the display of the console cursor.
 */
static void conge_disable_cursor(conge_ctx* ctx)
{
  CONSOLE_CURSOR_INFO info;

  info.dwSize = 100;
  info.bVisible = FALSE;

  SetConsoleCursorInfo(ctx->internal.output, &info);
}

/*
 * Used internally to get the console window's size.
 */
static void conge_get_window_size(conge_ctx* ctx)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(ctx->internal.output, &csbi);

  ctx->cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  ctx->rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

/*
 * For internal use: move the cursor unless putchar could do it.
 */
static void conge_move_cursor_to(conge_ctx* ctx, int x, int y)
{
  /* Writing to stdout moves the cursor right anyways. */
  if (ctx->internal.cursor_y != y || ctx->internal.cursor_x != x - 1)
    {
      COORD coord;

      coord.X = x;
      coord.Y = y;

      SetConsoleCursorPosition(ctx->internal.output, coord);
    }

  ctx->internal.cursor_x = x;
  ctx->internal.cursor_y = y;
}

/*
 * For internal use: print all subsequent characters in this color.
 */
static void conge_set_text_color(conge_ctx* ctx, int color)
{
  if (ctx->internal.last_color != color)
    {
      SetConsoleTextAttribute(ctx->internal.output, color);
      ctx->internal.last_color = color;
    }
}

#define CONGE_STDOUT 1

/*
 * Internal: draw the current frame with double-buffering.
 */
static void conge_draw_frame(conge_ctx* ctx)
{
  int x, y;

  /* Kludge: trigger the initial cursor movement. */
  ctx->internal.cursor_x = -2;
  ctx->internal.cursor_y = 0;

  for (y = 0; y < ctx->rows; y++)
    for (x = 0; x < ctx->cols; x++)
      {
        conge_pixel* front = conge_get_pixel(ctx, x, y);
        conge_pixel* back = &ctx->internal.backbuffer[ctx->rows * x + y];

        /* If front and back pixels differ, print the front one. */
        if (front->character != back->character
            || front->fg != back->fg || front->bg != back->bg)
          {
            conge_move_cursor_to(ctx, x, y);
            conge_set_text_color(ctx, (16 * front->bg) + front->fg);
            _write(CONGE_STDOUT, &front->character, 1);

            *back = *front;
          }
      }

  /* Prevent visual glitches. */
  conge_move_cursor_to(ctx, 0, 0);
}

#undef CONGE_STDOUT

#define CONGE_INPUT_BUFFER 10

/*
 * Internal: populate the input-related variables.
 */
static void conge_handle_input(conge_ctx* ctx)
{
  int i;

  INPUT_RECORD records[CONGE_INPUT_BUFFER];
  DWORD count;

  ctx->scroll = 0;

  GetNumberOfConsoleInputEvents(ctx->internal.input, &count);

  if (!count)
    return;

  ReadConsoleInput(ctx->internal.input, records, CONGE_INPUT_BUFFER, &count);

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
              ctx->scroll = scroll / abs(scroll);
            }

          ctx->buttons = event.dwButtonState;
        }
    }
}

#undef CONGE_INPUT_BUFFER

/*
 * Run the conge mainloop with a maximum FPS, calling TICK every frame.
 */
static void conge_run(conge_ctx* ctx, conge_callback tick, int max_fps)
{
  const double min_delta = 1.0 / max_fps;

  struct timeb start, end; /* used for measuring delta */

  /* FPS measurement. */
  int frames_count = 0;
  double second = 0.0;

  int screen_area = 0; /* used to detect changes in resolution */
  int buffer_size, prev_buffer_size = 0;

  /* Enable mouse support. */
  DWORD mouse_flags = ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
  SetConsoleMode(ctx->internal.input, mouse_flags);

  while (!ctx->exit)
    {
      int i;

      ftime(&start);

      conge_get_window_size(ctx);

      screen_area = ctx->rows * ctx->cols;
      buffer_size = screen_area * sizeof(conge_pixel);

      ctx->frame = malloc(buffer_size);
      ctx->internal.backbuffer = realloc(ctx->internal.backbuffer, buffer_size);

      /* Clear the screen. */
      for (i = 0; i < screen_area; i++)
        ctx->frame[i] = conge_empty;

      /* Force a redraw when the window size changes. */
      if (prev_buffer_size != buffer_size)
        {
          conge_disable_cursor(ctx); /* the cursor reactivates after a resize */
          memset(ctx->internal.backbuffer, 0, buffer_size); /* fill with junk */
          prev_buffer_size = buffer_size;
        }

      conge_handle_input(ctx);
      tick(ctx);
      conge_draw_frame(ctx);

      /* Dispose of the unneeded frame. */
      free(ctx->frame);
      ctx->frame = NULL;

      ftime(&end);

      /* Measure the delta time in seconds. */
      ctx->delta = (end.time - start.time) + 0.001 * (end.millitm - start.millitm);

      /* Sleep in order to prevent the game from running too quickly. */
      if (ctx->delta < min_delta)
        {
          Sleep(1000 * (min_delta - ctx->delta));
          ctx->delta = min_delta;
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

  free(ctx->internal.backbuffer);
  ctx->internal.backbuffer = NULL;

  /* Fix colors in the console before exiting. */
  conge_set_text_color(ctx, CONGE_WHITE);
}

// clang-format on
