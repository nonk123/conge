/*
 * conge.hpp - the C++ ConGE wrapper.
 */

#ifndef CONGE_HPP
#define CONGE_HPP

// Silence some stupid warnings.
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

#include <string>

extern "C"
{
#include "conge.h"
}

namespace Conge
{
  /*
   * A simple object-oriented wrapper over conge_pixel.
   */
  class Pixel
  {
  private:
    conge_pixel value;

  public:
    Pixel (unsigned char character, int fg, int bg)
      : value (conge_new_pixel (character, fg, bg))
    {
    }

    Pixel (conge_pixel pixel) : value (pixel)
    {
    }

    /*
     * Return the numeric value acceptable by the C functions.
     */
    conge_pixel get_value ()
    {
      return value;
    }

    unsigned char get_character ()
    {
      return conge_get_character (value);
    }

    void set_character (unsigned char character)
    {
      conge_set_character (&value, character);
    }

    int get_fg ()
    {
      return conge_get_fg (value);
    }

    void set_fg (int fg)
    {
      conge_set_fg (&value, fg);
    }

    int get_bg ()
    {
      return conge_get_bg (value);
    }

    void set_bg (int bg)
    {
      conge_set_bg (&value, bg);
    }
  };

  /*
   * A wrapper over the conge_* functions available in tick ().
   */
  class App
  {
  private:
    conge_ctx* ctx;

    // A global variable used for wrapping the tick function.
    static App* acting_instance;

  public:
    App ()
    {
    }

  private:
    /*
     * The member function wrapper which can be used in the C API.
     */
    static void tick_wrapper (conge_ctx* ctx)
    {
      acting_instance->tick ();
    }

  public:
    /*
     * Run the ConGE application.
     *
     * This will start calling tick() at most MAX_FPS times per second.
     */
    int run (int max_fps)
    {
      ctx = conge_init ();

      acting_instance = this;
      int exit = conge_run (ctx, &App::tick_wrapper, max_fps);
      acting_instance = nullptr;

      conge_free (ctx);
      ctx = nullptr;

      return exit;
    }

    /*
     * This method will be called every frame once the application is started.
     */
    virtual void tick () = 0;

  private:
    /*
     * The wrapper functions must assert this to prevent suspicious calls.
     */
    bool is_running ()
    {
      return ctx != nullptr && acting_instance != nullptr;
    }

  public:
    int get_width ()
    {
      return ctx->cols;
    }

    int get_height ()
    {
      return ctx->rows;
    }

    Pixel get_pixel (int x, int y)
    {
      Pixel deflt (' ', CONGE_WHITE, CONGE_BLACK);

      if (!is_running ())
        return deflt;

      conge_pixel* pixel = conge_get_pixel (ctx, x, y);

      if (pixel == nullptr)
        return deflt;
      else
        return Pixel (*pixel);
    }

    void set_pixel (int x, int y, Pixel pixel)
    {
      if (!is_running ())
        return;

      conge_pixel* current = conge_get_pixel (ctx, x, y);

      if (current != NULL)
        *current = pixel.get_value ();
    }

    unsigned char get_character (int x, int y)
    {
      return get_pixel (x, y).get_character ();
    }

    void set_character (int x, int y, unsigned char character)
    {
      Pixel pixel = get_pixel (x, y);
      pixel.set_character (character);
      set_pixel (x, y, pixel);
    }

    int get_fg (int x, int y)
    {
      return get_pixel (x, y).get_fg ();
    }

    void set_fg (int x, int y, int fg)
    {
      Pixel pixel = get_pixel (x, y);
      pixel.set_fg (fg);
      set_pixel (x, y, pixel);
    }

    int get_bg (int x, int y)
    {
      return get_pixel (x, y).get_bg ();
    }

    void set_bg (int x, int y, int bg)
    {
      Pixel pixel = get_pixel (x, y);
      pixel.set_bg (bg);
      set_pixel (x, y, pixel);
    }

    void set_title (std::string title)
    {
      std::strcpy (ctx->title, title.c_str ());
    }

    void request_exit ()
    {
      if (is_running ())
        ctx->exit = true;
    }

    void cancel_exit ()
    {
      if (is_running ())
        ctx->exit = false;
    }

    void request_grab ()
    {
      if (is_running ())
        ctx->grab = true;
    }

    void cancel_grab ()
    {
      if (is_running ())
        ctx->grab = false;
    }

    bool is_key_down (int scancode)
    {
      return is_running () && conge_is_key_down (ctx, scancode);
    }

    bool is_key_just_pressed (int scancode)
    {
      return is_running () && conge_is_key_just_pressed (ctx, scancode);
    }

    bool is_button_down (int mouse_button)
    {
      return is_running () && conge_is_button_down (ctx, mouse_button);
    }

    /*
     * Measured in characters on the screen.
     */
    int get_mouse_x ()
    {
      return is_running () ? ctx->mouse_x : -1;
    }

    /*
     * Measured in characters on the screen.
     */
    int get_mouse_y ()
    {
      return is_running () ? ctx->mouse_y : -1;
    }

    /*
     * Return mouse position relative to the previous frame.
     *
     * Only useful during mouse grab.
     */
    int get_relative_x ()
    {
      return is_running () ? ctx->mouse_dx : 0;
    }

    /*
     * Return mouse position relative to the previous frame.
     *
     * Only useful during mouse grab.
     */
    int get_relative_y ()
    {
      return is_running () ? ctx->mouse_dy : 0;
    }

    /*
     * The return value describes scroll direction: 1 is forward, -1 is
     * backward, and 0 means no scrolling.
     */
    int get_scroll ()
    {
      return is_running () ? ctx->scroll : 0;
    }

    /*
     * Synonym for set_pixel which comes from the safer C function.
     */
    void fill (int x, int y, Pixel pixel)
    {
      set_pixel (x, y, pixel);
    }

    /*
     * Draw a line between two specified points.
     */
    void line (int x1, int y1, int x2, int y2, Pixel fill)
    {
      if (is_running ())
        conge_draw_line (ctx, x1, y1, x2, y2, fill.get_value ());
    }

    /*
     * Fill a triangle defined by the three of its vertices in counter-
     * clockwise order.
     */
    void fill_triangle (int x1, int y1, int x2, int y2, int x3, int y3, Pixel fill)
    {
      if (is_running ())
        conge_fill_triangle (ctx, x1, y1, x2, y2, x3, y3, fill.get_value ());
    }

    /*
     * Draw some text at specified position, with specified color.
     */
    void write_string (std::string string, int x, int y, int fg, int bg)
    {
      if (is_running ())
        conge_write_string (ctx, string.c_str (), x, y, fg, bg);
    }

    int get_fps ()
    {
      return is_running () ? ctx->fps : 0;
    }

    double get_delta ()
    {
      return is_running () ? ctx->delta : 0.0;
    }

    double get_timestep ()
    {
      return is_running () ? ctx->timestep : 0.0;
    }
  };

  App* App::acting_instance = nullptr;
}

#endif /* CONGE_HPP */
