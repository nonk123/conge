#include "conge.h"

conge_pixel*
conge_get_pixel (conge_ctx* ctx, int x, int y)
{
  if (ctx == NULL)
    return NULL;
  else if (x < 0 || y < 0 || x >= ctx->cols || y >= ctx->rows)
    return NULL;
  else
    return &ctx->frame[ctx->rows * x + y];
}

int
conge_fill (conge_ctx* ctx, int x, int y, conge_pixel pixel)
{
  if (ctx == NULL)
    return 1;

  if (x >= 0 && y >= 0 && x < ctx->cols && y < ctx->rows)
    *conge_get_pixel (ctx, x, y) = pixel;

  return 0;
}

int
conge_draw_line (conge_ctx* ctx, int x0, int y0, int x1, int y1, conge_pixel fill)
{
  int i = 0;

  double x = x0, y = y0;
  double dx = x1 - x0, dy = y1 - y0;
  double step = CONGE_MAX (fabs (dx), fabs (dy));

  if (ctx == NULL)
    return 1;

  /* Special case: just a dot. */
  if (step <= 0.01)
    {
      conge_fill (ctx, x0, y0, fill);
      return 0;
    }

  dx /= step;
  dy /= step;

  while (i++ < step)
    {
      conge_fill (ctx, round (x), round (y), fill);
      x += dx;
      y += dy;
    }

  return 0;
}

int
conge_write_string (conge_ctx* ctx, char* string, int x, int y, conge_color fg, conge_color bg)
{
  int i;

  if (ctx == NULL)
    return 1;

  if (string == NULL)
    return 2;

  for (i = 0; i < strlen (string); i++)
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

  return 0;
}

/*
 * Internal: move the cursor unless putchar could do it.
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
 * Internal: print all subsequent characters in this color.
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

void
conge_draw_frame (conge_ctx* ctx)
{
  int x, y;

  /* Trigger the initial cursor movement. */
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
  conge_set_text_color (ctx, CONGE_WHITE);
}
