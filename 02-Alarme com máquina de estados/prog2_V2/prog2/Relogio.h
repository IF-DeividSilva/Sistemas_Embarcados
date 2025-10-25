#ifndef RELOGIO_H
#define RELOGIO_H

class Relogio {
  public:
    int hora, minuto, segundo;
    
    Relogio();
    void tick();
    void ajustar(int h, int m, int s);
};

#endif