#include "conge.h"

int
conge_is_key_pressed (conge_ctx* ctx, int code)
{
  int index, offset;

  if (ctx == NULL)
    return 0;

  index = code / (8 * sizeof (ctx->_keys));
  offset = code % (8 * sizeof (ctx->_keys));

  /* !! converts it to a "real" boolean value (0 or 1). */
  return !!(ctx->_keys[index] & (1 << offset));
}

int
conge_is_button_pressed (conge_ctx* ctx, int mask)
{
  if (ctx == NULL)
    return 0;
  else
    return ctx->_buttons & mask;
}

/*
 * Update mouse cursor positions (in pixels), and handle mouse grab.
 */
void
conge_process_mouse (conge_ctx* ctx)
{
  if (ctx->grab)
    {
      POINT mouse_p;
      RECT window;

      GetCursorPos (&mouse_p);
      GetWindowRect (ctx->_window, &window);

      /* Find the center of the console window. */
      int cx = window.left + (window.right - window.left) / 2;
      int cy = window.top  + (window.bottom - window.top) / 2;

      ctx->mouse_dx = mouse_p.x - cx;
      ctx->mouse_dy = mouse_p.y - cy;

      SetCursorPos (cx, cy);
    }
  else
    {
      ctx->mouse_dx = 0;
      ctx->mouse_dy = 0;
    }
}

void
conge_handle_input (conge_ctx* ctx)
{
  int i;

  INPUT_RECORD records[10];
  DWORD count;

  ctx->scroll = 0;

  conge_process_mouse (ctx);

  GetNumberOfConsoleInputEvents (ctx->_input, &count);

  if (!count)
    return;

  ReadConsoleInput (ctx->_input, records, 10, &count);

  for (i = 0; i < count; i++)
    {
      if (records[i].EventType == KEY_EVENT)
        {
          KEY_EVENT_RECORD event = records[i].Event.KeyEvent;
          WORD scancode = event.wVirtualScanCode;

          if (scancode < 256)
            {
              /* To make a 256-bit bitflag, we split it into 8 32-bit ints. */
              int index = scancode / (8 * sizeof (ctx->_keys));

              /* The remainder is just the offset of that bit in the flag. */
              int offset = scancode % (8 * sizeof (ctx->_keys));

              int mask = 1 << offset;

              /* Set/unset the bit. */
              if (event.bKeyDown)
                ctx->_keys[index] |= mask;
              else
                ctx->_keys[index] &= ~mask;
            }
        }
      else if (records[i].EventType == MOUSE_EVENT)
        {
          MOUSE_EVENT_RECORD event = records[i].Event.MouseEvent;

          ctx->mouse_x = event.dwMousePosition.X;
          ctx->mouse_y = event.dwMousePosition.Y;

          if (event.dwEventFlags & MOUSE_WHEELED)
            {
              int scroll = event.dwButtonState;
              ctx->scroll = scroll / abs (scroll); /* clamp between -1 and 1 */
            }
          else
            ctx->_buttons = event.dwButtonState;
        }
    }
}
