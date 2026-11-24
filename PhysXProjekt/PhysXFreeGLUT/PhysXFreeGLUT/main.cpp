
#include <GL/freeglut.h>

#include "../Gra/Gra.h"

#include <iostream>

// Glowny obiekt aplikacji
Gra gra;

// Rysowanie sceny
void wyswietl()
{
    gra.rysuj();
}

// Aktualizacja symulacji
void zegar(int)
{
    gra.aktualizuj();

    glutTimerFunc(16, zegar, 0);
}

// Zmiana rozmiaru okna
void zmienRozmiar(
    int szerokosc,
    int wysokosc)
{
    gra.zmienRozmiarOkna(
        szerokosc,
        wysokosc
    );
}

// Klawiatura
void klawiatura(
    unsigned char klawisz,
    int,
    int)
{
    gra.obsluzKlawisz(klawisz);
}

// Przyciski myszy
void mysz(
    int przycisk,
    int stan,
    int x,
    int y)
{
    gra.obsluzMysz(
        przycisk,
        stan,
        x,
        y
    );
}

// Przeciaganie myszy
void ruchMyszy(
    int x,
    int y)
{
    gra.obsluzRuchMyszy(x, y);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGBA |
        GLUT_DEPTH
    );

    glutInitWindowSize(1280, 720);

    glutSetOption(
        GLUT_ACTION_ON_WINDOW_CLOSE,
        GLUT_ACTION_GLUTMAINLOOP_RETURNS
    );

    glutCreateWindow(
        "Angry Birds - NVIDIA PhysX GPU"
    );

    // Inicjalizacja silnika i sceny.
    if (!gra.inicjalizuj())
    {
        std::cerr
            << "Nie mozna uruchomic gry.\n";

        gra.zakoncz();
        return 1;
    }

    // Funkcje FreeGLUT.
    glutDisplayFunc(wyswietl);
    glutReshapeFunc(zmienRozmiar);
    glutKeyboardFunc(klawiatura);

    // Nowe funkcje sterowania proca.
    glutMouseFunc(mysz);
    glutMotionFunc(ruchMyszy);

    glutTimerFunc(16, zegar, 0);

    std::cout << "\nSTEROWANIE:\n";
    std::cout << "LPM - chwyc ptaka\n";
    std::cout << "Przeciagnij - naciagnij proce\n";
    std::cout << "Pusc LPM - wystrzel\n";
    std::cout << "P - pauza\n";
    std::cout << "R - reset\n";
    std::cout << "ESC - wyjscie\n";

    glutMainLoop();

    gra.zakoncz();

    return 0;
}
