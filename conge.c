#include "conge.h"

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

  ctx->mouse_dx = 0;
  ctx->mouse_dy = 0;

  ctx->grab = 0;

  for (i = 0; i < CONGE_SCANCODE_COUNT; i++)
    ctx->keys[i] = 0;

  strcpy (ctx->title, "conge");

  ctx->frame = NULL;

  ctx->internal.input = GetStdHandle (STD_INPUT_HANDLE);
  ctx->internal.output = GetStdHandle (STD_OUTPUT_HANDLE);
  ctx->internal.c_window = GetConsoleWindow ();

  ctx->internal.cursor_x = 0;
  ctx->internal.cursor_y = 0;

  ctx->internal.last_color = 0;

  ctx->internal.backbuffer = NULL;

  return ctx;
}

void
conge_free (conge_ctx* ctx)
{
  if (ctx != NULL)
    {
      free (ctx);
      ctx = NULL;
    }
}

/*
 * Internal: disable the display of the console cursor.
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
 * Internal: update the window size variables.
 */
void
conge_get_window_size (conge_ctx* ctx)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo (ctx->internal.output, &csbi);

  ctx->cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  ctx->rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

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

  return exit;
}