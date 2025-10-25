#ifndef SIRENE_H
#define SIRENE_H

class Sirene {
  private:
    int pin;
  
  public:
    Sirene(int p);
    void tocar();
    void parar();
};

#endif