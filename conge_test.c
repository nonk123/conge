#include "conge.h"

static int x = 0, y = 0;

/* This will be called every frame before rendering it. */
void
my_tick (conge_ctx* ctx)
{
  /* Exit the game if escape is pressed. */
  if (ctx->keys[CONGE_ESC])
    {
      ctx->exit = 1;
      return;
    }

  /* Set the window title. */
  strcpy (ctx->title, "conge test");

  if (ctx->grab)
    {
      /* When the mouse is grabbed, we use relative mouse position to move. */
      x += round ( 0.1 * ctx->mouse_dx);
      y += round (0.05 * ctx->mouse_dy);
    }
  else if (ctx->buttons & CONGE_LMB) /* check if LMB is held */
    {
      /* Mouse position is measured in characters on the screen. */
      x = ctx->mouse_x;
      y = ctx->mouse_y;
    }
  else
    {
      /* Keyboard and scrolling example. */
      x += ctx->keys[CONGE_D] - ctx->keys[CONGE_A];
      y += ctx->keys[CONGE_S] - ctx->keys[CONGE_W] - ctx->scroll;
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

  /* Initiate mouse grab. */
  if (ctx->keys[CONGE_LALT])
    ctx->grab = !ctx->grab;

  {
    /* White rectangle. */
    conge_pixel fill = {' ', CONGE_BLACK, CONGE_WHITE};

    /* Helper functions. They won't draw outside of screen bounds. */
    conge_draw_line (ctx, 9, 9, 47, 14, fill);
    conge_draw_line (ctx, 6, 100, 6, 30, fill);
    conge_fill (ctx, 4, 4, fill);

    /* Direct access. Might segfault if X or Y are out of screen bounds. */
    conge_get_pixel (ctx, x, y)->character = '*';
  }

  /* Display a simple FPS counter. */
  {
    char fps_string[64];
    int y = ctx->rows - 1, x = ctx->cols;

    /* Get the current FPS and right-align it. */
    x -= sprintf (fps_string, "FPS: %d", ctx->fps);

    /* Write the FPS in the bottom-right corner of the screen. */
    conge_write_string (ctx, fps_string, x, y, CONGE_BLACK, CONGE_WHITE);
  }

  /*
   * NOTE: ctx->delta and ctx->timestep are not shown here.
   * The latter is recommended for physics as it is constant.
   */
}

int
main (void)
{
  int exit;
  conge_ctx* ctx = conge_init ();

  if (ctx == NULL)
    return 1;

  /*
   * Run my_tick 24 times a second.
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
