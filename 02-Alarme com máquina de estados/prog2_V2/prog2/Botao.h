#ifndef BOTAO_H
#define BOTAO_H

class Botao {
  private:
    int pin;
    unsigned long lastPress;
    const long debounce;
  
  public:
    Botao(int p);
    bool pressionado();
};

#endif