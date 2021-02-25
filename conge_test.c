#include "conge.h"

static int x = 0, y = 0;

/* This will be called every frame before rendering it. */
void my_tick(conge_ctx* ctx)
{
  /* Exit the game if escape is pressed. */
  if (ctx->keys[CONGE_ESC])
    {
      ctx->exit = 1;
      return;
    }

  /* Check if the left mouse button is pressed. */
  if (ctx->buttons & CONGE_LMB)
    {
      /* Mouse position is measured in characters on the screen. */
      x = ctx->mouse_x;
      y = ctx->mouse_y;
    }
  else
    {
      /* Keyboard handling example. */
      x -= ctx->keys[CONGE_A];
      x += ctx->keys[CONGE_D];
      y += ctx->keys[CONGE_S];
      y -= ctx->keys[CONGE_W];
    }

  /* Scrolling example. */
  y -= ctx->scroll;

  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;

  /* Screen size is accessed via the variables rows and cols. */
  if (x >= ctx->cols)
    x = ctx->cols - 1;
  if (y >= ctx->rows)
    y = ctx->rows - 1;

  /* This is how you would alter the pixels in the frame. */
  conge_get_pixel(ctx, x, y)->character = '*';
}

int main()
{
  conge_ctx* ctx = conge_init();

  /*
   * Run my_tick 24 times a second.
   * In a real game, you'd be using between 30 and 60 FPS.
   */
  conge_run(ctx, my_tick, 24);

  conge_free(ctx);

  return 0;
}
