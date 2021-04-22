#include <string>
#include <sstream>

#include "conge.hpp"

using namespace Conge;

// Subclass Conge::App to access the API.
class CongeTest : public App
{
private:
  int x, y;

public:
  CongeTest () : x (0), y (0)
  {
  }

  // This method will be called every frame.
  void tick ()
  {
    // Exit the game if escape is pressed.
    if (is_key_down (CONGE_ESC))
      {
        request_exit ();
        return;
      }

    // Set the window title.
    set_title ("ConGE test");

    if (is_button_down (CONGE_LMB)) // check if LMB is down
      {
        // Mouse position is measured in characters on the screen.
        x = get_mouse_x ();
        y = get_mouse_y ();
      }
    else
      {
        // Keyboard and scrolling example.
        x += is_key_down (CONGE_D);
        x -= is_key_down (CONGE_A);

        y += is_key_down (CONGE_S);
        y -= is_key_down (CONGE_W) + get_scroll ();
      }

    if (x < 0)
      x = 0;
    if (y < 0)
      y = 0;

    // Screen size is accessed via the following getters.
    if (x >= get_width ())
      x = get_width () - 1;
    if (y >= get_height ())
      y = get_height () - 1;

    // White rectangle.
    Pixel rect (' ', CONGE_BLACK, CONGE_WHITE);

    // Helper functions. They won't draw outside of screen bounds.
    fill (4, 4, rect);
    line (9, 9, 47, 14, rect);
    line (6, 100, 6, 30, rect);
    fill_triangle (40, 40, 50, 30, 30, 30, rect);

    // Altering a pixel's properties.
    set_character (x, y, '*');

    // Format a simple FPS counter.
    std::ostringstream stream;
    stream << "FPS: " << get_fps ();

    std::string fps = stream.str ();

    int fps_x = get_width () - fps.length ();
    int fps_y = get_height () - 1;

    // Write the FPS in the bottom-right corner of the screen.
    write_string (fps, fps_x, fps_y, CONGE_BLACK, CONGE_WHITE);
  }
};

int main ()
{
  CongeTest test;
  return test.run (24); // run test.tick at most 24 times per second
}
