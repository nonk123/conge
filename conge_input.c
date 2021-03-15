#include "conge.h"

/*
 * Update mouse cursor positions (in pixels), and handle mouse grab.
 */
void
conge_process_mouse (conge_ctx* ctx)
{
  /* While the mouse is grabbed, keep moving the mouse cursor to the center of
     the console window. */
  if (ctx->grab)
    {
      POINT mouse_p;
      RECT window;

      GetCursorPos (&mouse_p);
      GetWindowRect (ctx->internal.c_window, &window);

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

  GetNumberOfConsoleInputEvents (ctx->internal.input, &count);

  if (!count)
    return;

  ReadConsoleInput (ctx->internal.input, records, 10, &count);

  for (i = 0; i < count; i++)
    {
      if (records[i].EventType == KEY_EVENT)
        {
          KEY_EVENT_RECORD event = records[i].Event.KeyEvent;
          WORD scancode = event.wVirtualScanCode;

          if (scancode < CONGE_SCANCODE_COUNT)
            ctx->keys[scancode] = event.bKeyDown;
        }
      else if (records[i].EventType == MOUSE_EVENT)
        {
          MOUSE_EVENT_RECORD event = records[i].Event.MouseEvent;

          ctx->mouse_x = event.dwMousePosition.X;
          ctx->mouse_y = event.dwMousePosition.Y;

          if (event.dwEventFlags & MOUSE_WHEELED)
            {
              int scroll = event.dwButtonState;
              ctx->scroll = scroll / abs (scroll);
            }

          ctx->buttons = event.dwButtonState;
        }
    }
}
