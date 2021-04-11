#include "conge.h"

conge_pixel
conge_new_pixel (char character, int fg, int bg)
{
  /* Squash two 16-color variables into 1 byte. */
  fg = (fg & 0xF) << 8;
  bg = (bg & 0xF) << 12;

  return (conge_pixel) character | fg | bg;
}

char
conge_get_character (conge_pixel pixel)
{
  return (pixel >> 0) & 0xFF; /* the first byte */
}

void
conge_set_character (conge_pixel* pixel, char character)
{
  if (pixel != NULL && character >= 32)
    *pixel = (conge_pixel) character | (*pixel & 0xFF00);
}

int
conge_get_fg (conge_pixel pixel)
{
  return (pixel >> 8) & 0xF; /* bits 8-11 */
}

void
conge_set_fg (conge_pixel* pixel, int fg)
{
  if (pixel != NULL && fg >= 0 && fg < 16)
    *pixel = (conge_pixel) (fg << 8) | (*pixel & 0xF0FF);
}

int
conge_get_bg (conge_pixel pixel)
{
  return (pixel >> 12) & 0xF; /* bits 12-15 */
}

void
conge_set_bg (conge_pixel* pixel, int bg)
{
  if (pixel != NULL && bg >= 0 && bg < 16)
    *pixel = (conge_pixel) (bg << 12) | (*pixel & 0xFFF);
}

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
conge_fill (conge_ctx* ctx, int x, int y, conge_pixel fill)
{
  conge_pixel* pixel;

  if (ctx == NULL)
    return 1;

  pixel = conge_get_pixel (ctx, x, y);

  if (pixel != NULL)
    *pixel = fill;

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

  /* Special case: just a dot. Prevents division by zero. */
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

/*
 * Check if point (x2; y2) is within the triangle edge (x0; y0)--(x1; y1).
 */
int
conge_check_edge (double x0, double y0, double x1, double y1, double x2, double y2)
{
  return (x2 - x0) * (y1 - y0) - (y2 - y0) * (x1 - x0) >= 0.0;
}

int
conge_fill_triangle (conge_ctx* ctx, int x0, int y0, int x1, int y1,
                     int x2, int y2, conge_pixel fill)
{
  int bx0, by0, bx1, by1; /* bounding box */
  int x, y;

  if (ctx == NULL)
    return 1;

  bx0 = CONGE_MIN (x0, CONGE_MIN (x1, x2));
  by0 = CONGE_MIN (y0, CONGE_MIN (y1, y2));
  bx1 = CONGE_MAX (x0, CONGE_MAX (x1, x2));
  by1 = CONGE_MAX (y0, CONGE_MAX (y1, y2));

  for (x = bx0; x <= bx1; x++)
    for (y = by0; y <= by1; y++)
      {
        int inside = 1;

        inside &= conge_check_edge (x0, y0, x1, y1, x, y);
        inside &= conge_check_edge (x1, y1, x2, y2, x, y);
        inside &= conge_check_edge (x2, y2, x0, y0, x, y);

        if (inside)
          conge_fill (ctx, x, y, fill);
      }

  return 0;
}

int
conge_write_string (conge_ctx* ctx, char* string, int x, int y, int fg, int bg)
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

          conge_set_character (pixel, string[i]);
          conge_set_fg (pixel, fg);
          conge_set_bg (pixel, bg);
        }
    }

  return 0;
}

/*
 * Move the cursor unless putchar can do it.
 */
void
conge_move_cursor_to (conge_ctx* ctx, int x, int y)
{
  /* Writing to stdout moves the cursor to the right anyways. */
  if (ctx->_cursor_y != y || ctx->_cursor_x != x - 1)
    {
      COORD coord;

      coord.X = x;
      coord.Y = y;

      SetConsoleCursorPosition (ctx->_output, coord);
    }

  ctx->_cursor_x = x;
  ctx->_cursor_y = y;
}

/*
 * Print all subsequent characters in this color.
 */
void
conge_set_text_color (conge_ctx* ctx, int color)
{
  if (ctx->_last_color != color)
    {
      SetConsoleTextAttribute (ctx->_output, color);
      ctx->_last_color = color;
    }
}

void
conge_draw_frame (conge_ctx* ctx)
{
  int x, y;

  /* Trigger the initial cursor movement. */
  ctx->_cursor_x = -2;
  ctx->_cursor_y = 0;

  /* Compare the front and back buffers. */
  for (y = 0; y < ctx->rows; y++)
    for (x = 0; x < ctx->cols; x++)
      {
        conge_pixel* front = conge_get_pixel (ctx, x, y);
        conge_pixel* back = &ctx->_backbuffer[ctx->rows * x + y];

        if (*front != *back)
          {
            char character = conge_get_character (*front);
            int color = (16 * conge_get_bg (*front)) + conge_get_fg (*front);

            /* Write the pixel at that position. */
            conge_move_cursor_to (ctx, x, y);
            conge_set_text_color (ctx, color);
            write (1, &character, 1);

            *back = *front;
          }
      }

  /* Prevent visual glitches on exit. */
  conge_move_cursor_to (ctx, 0, 0);
  conge_set_text_color (ctx, CONGE_WHITE);
}
