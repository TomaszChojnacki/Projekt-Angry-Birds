
#pragma once

// Przechowuje wynik aktualnej rozgrywki.
class SystemPunktacji
{
public:
    void resetuj();

    void dodajPunktyZaSwinie(int liczbaSwin);

    void dodajBonusZaPtaki(int liczbaPtakow);

    int pobierzWynik() const;

private:
    int wynik = 0;

    static constexpr int PUNKTY_ZA_SWINIE = 1000;
    static constexpr int BONUS_ZA_PTAKA = 500;
};
