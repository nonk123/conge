#include <stdlib.h>
#include <stdio.h>
#include <sys/timeb.h>

#include <windows.h>

// clang-format off

typedef unsigned char color_t;

typedef struct conge_pixel {
  unsigned char character;
  color_t fg, bg; /* character and background color */
} conge_pixel;

typedef conge_pixel* conge_frame;

typedef struct conge_ctx {
  /* Public API. */
  conge_frame frame; /* the frame being rendered */
  int rows, cols; /* read-only: window size in characters */
  double delta; /* read-only: previous frame's delta time */
  char exit; /* when set to true, the program will exit */
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

enum conge_color {
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

/* The empty, "clear" pixel. */
conge_pixel conge_empty = {' ', CONGE_BLACK, CONGE_BLACK};

static conge_ctx* conge_init()
{
  conge_ctx* ctx = malloc(sizeof(conge_ctx));

  ctx->exit = 0;
  ctx->fps = 0;

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
  /* putchar can move the cursor to the right on the same line. */
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

/*
 * Draw the current frame with double-buffering.
 */
static void conge_draw_frame(conge_ctx* ctx)
{
  int x, y;

  for (y = 0; y < ctx->rows; y++)
    for (x = 0; x < ctx->cols; x++)
      {
        conge_pixel* pixel = conge_get_pixel(ctx, x, y);
        conge_pixel* back = &ctx->internal.backbuffer[ctx->rows * x + y];

        /* If the front and back pixels differ, we print the front one. */
        if (pixel->character != back->character
            || pixel->fg != back->fg || pixel->bg != back->bg)
          {
            conge_move_cursor_to(ctx, x, y);
            conge_set_text_color(ctx, (16 * pixel->bg) + pixel->fg);
            putchar(pixel->character);
          }
      }
}

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

  conge_disable_cursor(ctx);

  while (!ctx->exit)
    {
      int i;

      ftime(&start);

      conge_get_window_size(ctx);

      screen_area = ctx->rows * ctx->cols;
      buffer_size = screen_area * sizeof(conge_pixel);

      ctx->frame = malloc(buffer_size);
      ctx->internal.backbuffer = realloc(ctx->internal.backbuffer, buffer_size);

      for (i = 0; i < screen_area; i++)
        ctx->frame[i] = conge_empty;

      /* Force a redraw when the window size changes. */
      if (prev_buffer_size != buffer_size)
        {
          memset(ctx->internal.backbuffer, 0, buffer_size); /* fill with junk */
          prev_buffer_size = buffer_size;
        }

      tick(ctx);
      conge_draw_frame(ctx);

      /* Save the current frame in the backbuffer. */
      memcpy(ctx->internal.backbuffer, ctx->frame, buffer_size);
      free(ctx->frame);
      ctx->frame = NULL;

      ftime(&end);

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
