
#include "SystemPunktacji.h"

#include <algorithm>

void SystemPunktacji::resetuj()
{
    wynik = 0;
}

void SystemPunktacji::dodajPunktyZaSwinie(
    int liczbaSwin)
{
    wynik +=
        std::max(0, liczbaSwin) *
        PUNKTY_ZA_SWINIE;
}

void SystemPunktacji::dodajBonusZaPtaki(
    int liczbaPtakow)
{
    wynik +=
        std::max(0, liczbaPtakow) *
        BONUS_ZA_PTAKA;
}

int SystemPunktacji::pobierzWynik() const
{
    return wynik;
}
