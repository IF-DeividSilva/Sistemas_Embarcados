#ifndef JOYSTICK_H
#define JOYSTICK_H

class Joystick {
  private:
    int pinX, pinY;
    unsigned long lastReadX, lastReadY;
    const long debounce;
  
  public:
    Joystick(int x, int y);
    int lerX();
    int lerY();
};

#endif