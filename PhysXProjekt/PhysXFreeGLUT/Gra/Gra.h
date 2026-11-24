
#pragma once

#include "../Silnik/SilnikFizyki.h"
#include "../Silnik/Renderer.h"

#include "SystemPunktacji.h"

// Aktualny stan poziomu.
enum class StanRozgrywki
{
    Rozgrywka,
    Zwyciestwo,
    Przegrana
};

class Gra
{
public:
    bool inicjalizuj();

    void aktualizuj();
    void rysuj();

    void zmienRozmiarOkna(
        int szerokosc,
        int wysokosc
    );

    void obsluzKlawisz(
        unsigned char klawisz
    );

    void obsluzMysz(
        int przycisk,
        int stan,
        int x,
        int y
    );

    void obsluzRuchMyszy(
        int x,
        int y
    );

    void zakoncz();

private:
    SilnikFizyki silnikFizyki;
    Renderer renderer;
    SystemPunktacji punktacja;

    StanRozgrywki stanGry =
        StanRozgrywki::Rozgrywka;

    bool pauza = false;
    bool naciaganie = false;

    int pozostalePtaki = 5;

    int naliczoneSwinie = 0;

    physx::PxVec3 pozycjaPtaka =
        physx::PxVec3(-12.0f, 2.4f, 0.0f);

    physx::PxRigidDynamic* ostatniPtak = nullptr;

    float czasOdWystrzalu = 0.0f;

    static constexpr float KROK_FIZYKI =
        1.0f / 60.0f;

    static constexpr float MAKS_NACIAG = 3.0f;
    static constexpr float SILA_WYSTRZALU = 9.0f;

    bool czyMoznaStrzelac() const;

    void aktualizujNaciaganie(
        int x,
        int y
    );

    void wystrzel();
    void przygotujKolejnegoPtaka();
    void resetujGre();

    // Nowe metody:
    void sprawdzPunktacje();
    void sprawdzKoniecPoziomu();

    const char* pobierzNapisStanu() const;
};
