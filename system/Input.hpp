#ifndef INPUT_HPP
#define INPUT_HPP

struct Input {
  Input() : last_x_pos(0), last_y_pos(0), x_pos(0), y_pos(0), left_button_down(false), right_button_down(false)
  { 
    ZeroMemory(key_status, sizeof(key_status));  
  }
  bool key_status[256];
  // mouse status
  uint16_t  last_x_pos;
  uint16_t  last_y_pos;

  uint16_t  x_pos;
  uint16_t  y_pos;

  bool  left_button_down;
  bool  right_button_down;
};


#endif // #ifndef INPUT_HPP
