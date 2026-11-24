
#include "Gra.h"

#include <GL/freeglut.h>

#include <iostream>

using namespace physx;

namespace
{
    const PxVec3 POZYCJA_PROCY(
        -12.0f, 2.4f, 0.0f
    );

    constexpr float ODLEGLOSC_CHWYTANIA = 0.95f;
    constexpr float MINIMALNY_NACIAG = 0.15f;

    constexpr float CZAS_MINIMALNY_STRZALU = 1.5f;
    constexpr float CZAS_MAKSYMALNY_STRZALU = 5.0f;
}

bool Gra::inicjalizuj()
{
    std::cout << "Uruchamianie Angry Birds - Etap 4\n";

    if (!silnikFizyki.inicjalizuj())
    {
        std::cerr << "Blad inicjalizacji PhysX.\n";
        return false;
    }

    renderer.inicjalizuj();

    punktacja.resetuj();
    stanGry = StanRozgrywki::Rozgrywka;

    pozostalePtaki = 5;
    naliczoneSwinie = 0;

    pozycjaPtaka = POZYCJA_PROCY;
    ostatniPtak = nullptr;

    naciaganie = false;
    pauza = false;
    czasOdWystrzalu = 0.0f;

    return true;
}

bool Gra::czyMoznaStrzelac() const
{
    return
        stanGry == StanRozgrywki::Rozgrywka &&
        !pauza &&
        ostatniPtak == nullptr &&
        pozostalePtaki > 0;
}

void Gra::sprawdzPunktacje()
{
    // PhysX informuje nas o liczbie
    // wszystkich wyeliminowanych swin.
    const int wyeliminowane =
        silnikFizyki.pobierzWyeliminowaneSwinie();

    const int noweEliminacje =
        wyeliminowane - naliczoneSwinie;

    if (noweEliminacje > 0)
    {
        punktacja.dodajPunktyZaSwinie(
            noweEliminacje
        );

        naliczoneSwinie = wyeliminowane;

        std::cout
            << "Wyeliminowano swinie! Wynik: "
            << punktacja.pobierzWynik()
            << "\n";
    }
}

void Gra::sprawdzKoniecPoziomu()
{
    if (stanGry != StanRozgrywki::Rozgrywka)
        return;

    // Wygrana: wszystkie swinie wyeliminowane.
    if (silnikFizyki.pobierzLiczbeSwin() == 0)
    {
        stanGry = StanRozgrywki::Zwyciestwo;

        // Bonus tylko raz, przy przejsciu
        // ze stanu Rozgrywka do Zwyciestwo.
        punktacja.dodajBonusZaPtaki(
            pozostalePtaki
        );

        naciaganie = false;

        std::cout
            << "\nPOZIOM UKONCZONY!\n"
            << "Wynik: "
            << punktacja.pobierzWynik()
            << "\n";

        return;
    }

    // Przegrana dopiero po zakonczeniu
    // lotu ostatniego ptaka.
    if (pozostalePtaki == 0 &&
        ostatniPtak == nullptr)
    {
        stanGry = StanRozgrywki::Przegrana;

        std::cout
            << "\nPRZEGRANA!\n"
            << "Nacisnij R, aby zresetowac.\n";
    }
}

void Gra::aktualizuj()
{
    if (!pauza &&
        stanGry == StanRozgrywki::Rozgrywka)
    {
        silnikFizyki.aktualizuj(KROK_FIZYKI);

        sprawdzPunktacje();
        sprawdzKoniecPoziomu();

        if (stanGry == StanRozgrywki::Rozgrywka &&
            ostatniPtak)
        {
            czasOdWystrzalu += KROK_FIZYKI;

            const PxVec3 predkosc =
                ostatniPtak->getLinearVelocity();

            const bool zatrzymalSie =
                ostatniPtak->isSleeping() ||
                predkosc.magnitudeSquared() < 0.25f;

            const bool minalMinimalnyCzas =
                czasOdWystrzalu >=
                CZAS_MINIMALNY_STRZALU;

            const bool przekroczonoMaksymalnyCzas =
                czasOdWystrzalu >=
                CZAS_MAKSYMALNY_STRZALU;

            if ((minalMinimalnyCzas && zatrzymalSie) ||
                przekroczonoMaksymalnyCzas)
            {
                przygotujKolejnegoPtaka();
            }
        }

        // Po zakonczeniu lotu ostatniego ptaka
        // mozemy wykryc przegrana.
        sprawdzKoniecPoziomu();
    }

    glutPostRedisplay();
}

const char* Gra::pobierzNapisStanu() const
{
    switch (stanGry)
    {
    case StanRozgrywki::Rozgrywka:
        return "TRWA GRA";

    case StanRozgrywki::Zwyciestwo:
        return "ZWYCIESTWO! POZIOM UKONCZONY";

    case StanRozgrywki::Przegrana:
        return "PRZEGRANA! NACISNIJ R";

    default:
        return "";
    }
}

void Gra::rysuj()
{
    const bool ptakNaProcy =
        stanGry == StanRozgrywki::Rozgrywka &&
        ostatniPtak == nullptr &&
        pozostalePtaki > 0;

    renderer.rysuj(
        silnikFizyki,
        pauza,
        ptakNaProcy,
        naciaganie,
        pozycjaPtaka,
        pozostalePtaki,
        punktacja.pobierzWynik(),
        pobierzNapisStanu()
    );
}

void Gra::zmienRozmiarOkna(
    int szerokosc,
    int wysokosc)
{
    renderer.zmienRozmiarOkna(
        szerokosc,
        wysokosc
    );
}

void Gra::aktualizujNaciaganie(
    int x,
    int y)
{
    if (!naciaganie)
        return;

    PxVec3 pozycjaMyszy =
        renderer.pikseleNaSwiat(x, y);

    pozycjaMyszy.z = 0.0f;

    PxVec3 wektorNaciagu =
        pozycjaMyszy - POZYCJA_PROCY;

    const float dlugosc =
        wektorNaciagu.magnitude();

    if (dlugosc > MAKS_NACIAG)
    {
        wektorNaciagu =
            wektorNaciagu.getNormalized() *
            MAKS_NACIAG;
    }

    pozycjaPtaka =
        POZYCJA_PROCY + wektorNaciagu;

    glutPostRedisplay();
}

void Gra::obsluzMysz(
    int przycisk,
    int stan,
    int x,
    int y)
{
    if (przycisk != GLUT_LEFT_BUTTON)
        return;

    if (stan == GLUT_DOWN)
    {
        if (!czyMoznaStrzelac())
            return;

        const PxVec3 pozycjaMyszy =
            renderer.pikseleNaSwiat(x, y);

        const PxVec3 roznica =
            pozycjaMyszy - pozycjaPtaka;

        if (roznica.magnitude() <=
            ODLEGLOSC_CHWYTANIA)
        {
            naciaganie = true;
            aktualizujNaciaganie(x, y);
        }
    }
    else if (stan == GLUT_UP)
    {
        if (!naciaganie)
            return;

        if (pauza)
        {
            naciaganie = false;
            pozycjaPtaka = POZYCJA_PROCY;
            return;
        }

        aktualizujNaciaganie(x, y);

        naciaganie = false;
        wystrzel();
    }
}

void Gra::obsluzRuchMyszy(
    int x,
    int y)
{
    aktualizujNaciaganie(x, y);
}

void Gra::wystrzel()
{
    if (!czyMoznaStrzelac())
        return;

    const PxVec3 wektorWystrzalu =
        POZYCJA_PROCY - pozycjaPtaka;

    if (wektorWystrzalu.magnitude() <
        MINIMALNY_NACIAG)
    {
        pozycjaPtaka = POZYCJA_PROCY;
        return;
    }

    const PxVec3 predkoscPoczatkowa =
        wektorWystrzalu * SILA_WYSTRZALU;

    ostatniPtak =
        silnikFizyki.wystrzelPtaka(
            pozycjaPtaka,
            predkoscPoczatkowa
        );

    if (!ostatniPtak)
    {
        std::cerr
            << "Nie udalo sie utworzyc ptaka.\n";

        pozycjaPtaka = POZYCJA_PROCY;
        return;
    }

    --pozostalePtaki;
    czasOdWystrzalu = 0.0f;

    std::cout
        << "Wystrzelono ptaka. Pozostalo: "
        << pozostalePtaki
        << "\n";
}

void Gra::przygotujKolejnegoPtaka()
{
    // Poprzedni ptak pozostaje w scenie,
    // ale nie jest juz aktywnym strzalem.
    ostatniPtak = nullptr;

    pozycjaPtaka = POZYCJA_PROCY;
    czasOdWystrzalu = 0.0f;
    naciaganie = false;

    if (pozostalePtaki > 0)
    {
        std::cout << "Kolejny ptak gotowy.\n";
    }
}

void Gra::resetujGre()
{
    ostatniPtak = nullptr;

    silnikFizyki.wyczysc();

    if (!silnikFizyki.inicjalizuj())
    {
        std::cerr
            << "Blad ponownej inicjalizacji PhysX.\n";

        glutLeaveMainLoop();
        return;
    }

    punktacja.resetuj();

    stanGry = StanRozgrywki::Rozgrywka;

    pozostalePtaki = 5;
    naliczoneSwinie = 0;

    pauza = false;
    naciaganie = false;

    pozycjaPtaka = POZYCJA_PROCY;
    czasOdWystrzalu = 0.0f;
}

void Gra::obsluzKlawisz(
    unsigned char klawisz)
{
    switch (klawisz)
    {
    case 'p':
    case 'P':
        if (stanGry == StanRozgrywki::Rozgrywka)
        {
            pauza = !pauza;

            if (pauza && naciaganie)
            {
                naciaganie = false;
                pozycjaPtaka = POZYCJA_PROCY;
            }
        }
        break;

    case 'r':
    case 'R':
        resetujGre();
        break;

    case 27:
        glutLeaveMainLoop();
        break;
    }
}

void Gra::zakoncz()
{
    ostatniPtak = nullptr;
    silnikFizyki.wyczysc();
}
