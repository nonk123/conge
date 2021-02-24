#include "conge.h"

/* This will be called every frame before rendering it. */
void my_tick(conge_ctx* ctx)
{
  static int x = 0, y = 0;

  /* Initialize a pixel to fill. */
  conge_pixel star = {'*', CONGE_WHITE, CONGE_BLACK};

  /* This is how you would alter the pixels in the frame. */
  *conge_get_pixel(ctx, x++, y) = star;

  /* Screen size is accessed via these two fields: rows and cols. */
  if (x >= ctx->cols)
    {
      x = 0;
      y++;

      if (y >= ctx->rows)
        ctx->exit = 1; /* exit the game */
    }
}

int main()
{
  conge_ctx* ctx = conge_init();

  /*
   * Run my_tick 5 times a second.
   * In a real game, you'd be using between 30 and 60 FPS.
   */
  conge_run(ctx, my_tick, 5);

  conge_free(ctx);

  return 0;
}
