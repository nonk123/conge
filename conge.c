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

  ctx->mouse_x = 0;
  ctx->mouse_y = 0;

  ctx->mouse_dx = 0;
  ctx->mouse_dy = 0;

  ctx->grab = 0;

  strcpy (ctx->title, "ConGE");

  ctx->frame = NULL;

  ctx->_input = GetStdHandle (STD_INPUT_HANDLE);
  ctx->_output = GetStdHandle (STD_OUTPUT_HANDLE);
  ctx->_window = GetConsoleWindow ();

  for (i = 0; i < CONGE__KEYS_LENGTH; i++)
    ctx->_keys[i] = 0;

  ctx->_buttons = 0;

  ctx->_cursor_x = 0;
  ctx->_cursor_y = 0;

  ctx->_last_color = 0;

  ctx->_backbuffer = NULL;

  return ctx;
}

void
conge_free (conge_ctx* ctx)
{
  if (ctx == NULL)
    return;

  if (ctx->frame != NULL)
    {
      free (ctx->frame);
      ctx->frame = NULL;
    }

  if (ctx->_backbuffer != NULL)
    {
      free (ctx->_backbuffer);
      ctx->_backbuffer = NULL;
    }

  free (ctx);
  ctx = NULL;
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

  SetConsoleCursorInfo (ctx->_output, &info);
}

/*
 * Internal: update the window size variables.
 */
void
conge_get_window_size (conge_ctx* ctx)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo (ctx->_output, &csbi);

  ctx->cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  ctx->rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

int
conge_run (conge_ctx* ctx, conge_callback tick, int max_fps)
{
  struct timeb start, end; /* used for measuring delta */

  conge_pixel clear_pixel = conge_new_pixel (' ', CONGE_WHITE, CONGE_BLACK);

  /* FPS measurement. */
  int frames_count = 0;
  double second = 0.0;

  int screen_area = 0; /* used to detect changes in resolution */
  int buffer_size, prev_buffer_size = 0;

  int exit;

  if (ctx == NULL)
    return 1;

  if (max_fps < 1)
    return 2;

  ctx->timestep = 1.0 / max_fps;

  /* Enable mouse support. */
  DWORD mouse_flags = ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
  SetConsoleMode (ctx->_input, mouse_flags);

  for (;;)
    {
      int i;

      ftime (&start);

      /* The console window might've been resized last frame. */
      conge_get_window_size (ctx);

      /* Calculate screen buffer size. */
      screen_area = ctx->rows * ctx->cols;
      buffer_size = screen_area * sizeof (conge_pixel);

      /* Allocate the screen buffers. */
      ctx->frame = realloc (ctx->frame, buffer_size);
      ctx->_backbuffer = realloc (ctx->_backbuffer, buffer_size);

      /* Something went wrong in memory allocation. */
      if (ctx->frame == NULL || ctx->_backbuffer == NULL)
        {
          exit = 3;
          break;
        }

      /* Clear the screen. */
      for (i = 0; i < screen_area; i++)
        ctx->frame[i] = clear_pixel;

      /* Force a redraw when the window size changes. */
      if (prev_buffer_size != buffer_size)
        {
          conge_disable_cursor (ctx); /* the cursor reactivates after a resize */
          memset (ctx->_backbuffer, 0, buffer_size); /* fill with junk */
          prev_buffer_size = buffer_size;
        }

      conge_handle_input (ctx);
      tick (ctx);

      if (ctx->exit)
        {
          exit = 0;
          break;
        }

      SetConsoleTitle (ctx->title);

      conge_draw_frame (ctx);

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

      /* Update the FPS when one second passes. */
      if (second >= 1.0)
        {
          ctx->fps = frames_count;
          frames_count = 0;
          second -= 1.0;
        }
    }

  return exit;
}
