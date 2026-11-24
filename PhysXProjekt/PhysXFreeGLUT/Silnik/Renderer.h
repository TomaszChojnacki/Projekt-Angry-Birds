
#pragma once

#include "SilnikFizyki.h"

class Renderer
{
public:
    void inicjalizuj();

    void zmienRozmiarOkna(
        int szerokosc,
        int wysokosc
    );

    // Zamiana wspolrzednych myszy na wspolrzedne gry.
    physx::PxVec3 pikseleNaSwiat(
        int x,
        int y
    ) const;

    void rysuj(
        const SilnikFizyki& silnik,
        bool pauza,
        bool moznaStrzelac,
        bool naciaganie,
        const physx::PxVec3& pozycjaPtaka,
        int pozostalePtaki,
        int punkty,
        const char* stanGry
    );

private:
    int szerokoscOkna = 1280;
    int wysokoscOkna = 720;

    static constexpr float WYSOKOSC_KAMERY = 8.5f;
    static constexpr float POLOWA_WYSOKOSCI = 9.5f;

    void ustawKamere();


    // Wyswietla HP i ostatnie obrazenia
    void rysujEtykietyObiektow(
        const SilnikFizyki& silnik
    );

    void rysujObiekty(
        const SilnikFizyki& silnik
    );

    void rysujProce(
        const physx::PxVec3& pozycjaPtaka,
        bool pokazGume
    );

    void rysujPtaka(
        const physx::PxVec3& pozycja
    );

    void rysujTrajektorie(
        const physx::PxVec3& pozycja
    );

    void rysujInterfejs(
        const SilnikFizyki& silnik,
        bool pauza,
        int pozostalePtaki,
        int punkty,
        const char* stanGry
    );



    void rysujTekst(
        float x,
        float y,
        const char* napis
    );
};
