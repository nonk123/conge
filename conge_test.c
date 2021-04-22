#include "conge.h"

static int x = 0, y = 0;

/*
 * This function will be called every frame before rendering it.
 */
void
my_tick (conge_ctx* ctx)
{
  /* Exit the game if escape is pressed. */
  if (conge_is_key_down (ctx, CONGE_ESC))
    {
      ctx->exit = 1;
      return;
    }

  /* Set the window title. */
  strcpy (ctx->title, "ConGE test");

  if (conge_is_button_down (ctx, CONGE_LMB)) /* check if LMB is down */
    {
      /* Mouse position is measured in characters on the screen. */
      x = ctx->mouse_x;
      y = ctx->mouse_y;
    }
  else
    {
      /* Keyboard and scrolling example. */
      x += conge_is_key_down (ctx, CONGE_D);
      x -= conge_is_key_down (ctx, CONGE_A);

      y += conge_is_key_down (ctx, CONGE_S);
      y -= conge_is_key_down (ctx, CONGE_W) + ctx->scroll;
    }

  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;

  /* Screen size is accessed via the variables rows and cols. */
  if (x >= ctx->cols)
    x = ctx->cols - 1;
  if (y >= ctx->rows)
    y = ctx->rows - 1;

  {
    /* White rectangle. */
    conge_pixel fill = conge_new_pixel (' ', CONGE_BLACK, CONGE_WHITE);

    /* Helper functions. They won't draw outside of screen bounds. */
    conge_fill (ctx, 4, 4, fill);
    conge_draw_line (ctx, 9, 9, 47, 14, fill);
    conge_draw_line (ctx, 6, 100, 6, 30, fill);
    conge_fill_triangle (ctx, 40, 40, 50, 30, 30, 30, fill);

    /* Altering a pixel's properties. */
    conge_set_character (conge_get_pixel (ctx, x, y), '*');
  }

  /* Display a simple FPS counter. */
  {
    char fps_string[64];
    int y = ctx->rows - 1, x = ctx->cols;

    /* Get the current FPS and right-align it. */
    x -= sprintf (fps_string, "FPS: %2d", (int) ctx->fps);

    /* Write the FPS in the bottom-right corner of the screen. */
    conge_write_string (ctx, fps_string, x, y, CONGE_BLACK, CONGE_WHITE);
  }

  /*
   * NOTE: ctx->delta and ctx->timestep are not shown here.
   * The latter is recommended for physics as it is constant.
   */
}

int
main ()
{
  int exit;

  conge_ctx* ctx = conge_init ();

  /*
   * Run my_tick at most 24 times per second.
   * In a real game, you'd be using between 30 and 60 FPS.
   */
  exit = conge_run (ctx, my_tick, 24);

  if (ctx != NULL)
    {
      conge_free (ctx);
      ctx = NULL;
    }

  return exit;
}
