
#include "Renderer.h"

#include <GL/freeglut.h>
#include <GL/glu.h>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <iomanip>


using namespace physx;

namespace
{
    const PxVec3 POZYCJA_PROCY(
        -12.0f, 2.4f, 0.0f
    );

    constexpr float SILA_WYSTRZALU = 11.0f;
    constexpr float GRAWITACJA = 9.81f;
}

void Renderer::inicjalizuj()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    glClearColor(
        0.40f, 0.70f, 0.95f, 1.0f
    );
}

void Renderer::zmienRozmiarOkna(
    int szerokosc,
    int wysokosc)
{
    szerokoscOkna = std::max(1, szerokosc);
    wysokoscOkna = std::max(1, wysokosc);

    glViewport(
        0, 0,
        szerokoscOkna,
        wysokoscOkna
    );
}

PxVec3 Renderer::pikseleNaSwiat(
    int x,
    int y) const
{
    // Takie same proporcje jak w kamerze.
    const float proporcje =
        static_cast<float>(szerokoscOkna) /
        wysokoscOkna;

    const float polowaSzerokosci =
        POLOWA_WYSOKOSCI * proporcje;

    // W FreeGLUT punkt (0,0) jest w lewym
    // gornym rogu okna.
    const float nx =
        static_cast<float>(x) / szerokoscOkna;

    const float ny =
        static_cast<float>(y) / wysokoscOkna;

    const float swiatX =
        (nx * 2.0f - 1.0f) * polowaSzerokosci;

    const float swiatY =
        WYSOKOSC_KAMERY +
        (1.0f - ny * 2.0f) * POLOWA_WYSOKOSCI;

    return PxVec3(swiatX, swiatY, 0.0f);
}

void Renderer::ustawKamere()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    const double proporcje =
        static_cast<double>(szerokoscOkna) /
        wysokoscOkna;

    const double polowaSzerokosci =
        POLOWA_WYSOKOSCI * proporcje;

    glOrtho(
        -polowaSzerokosci,
        polowaSzerokosci,
        -POLOWA_WYSOKOSCI,
        POLOWA_WYSOKOSCI,
        0.1,
        100.0
    );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(
        0.0, WYSOKOSC_KAMERY, 35.0,
        0.0, WYSOKOSC_KAMERY, 0.0,
        0.0, 1.0, 0.0
    );
}

void Renderer::rysujObiekty(
    const SilnikFizyki& silnik)
{
    for (const ObiektSceny& obiekt :
        silnik.pobierzObiekty())
    {
        if (!obiekt.aktor)
            continue;

        PxTransform pozycja =
            obiekt.aktor->getGlobalPose();

        PxMat44 macierz(pozycja);

        glPushMatrix();
        glMultMatrixf(&macierz.column0.x);


        switch (obiekt.typ)
        {
        case TypObiektu::Podloze:
            glColor3f(0.27f, 0.55f, 0.22f);

            glScalef(
                obiekt.szerokosc,
                obiekt.wysokosc,
                obiekt.glebokosc
            );

            glutSolidCube(1.0);
            break;

        case TypObiektu::Drewno:
            glColor3f(0.72f, 0.45f, 0.21f);

            glScalef(
                obiekt.szerokosc,
                obiekt.wysokosc,
                obiekt.glebokosc
            );

            glutSolidCube(1.0);
            break;

        case TypObiektu::Szklo:
            // Jasnoniebieski material.
            glColor3f(0.50f, 0.85f, 0.95f);

            glScalef(
                obiekt.szerokosc,
                obiekt.wysokosc,
                obiekt.glebokosc
            );

            glutSolidCube(1.0);
            break;

        case TypObiektu::Kamien:
            // Szary material.
            glColor3f(0.55f, 0.58f, 0.63f);

            glScalef(
                obiekt.szerokosc,
                obiekt.wysokosc,
                obiekt.glebokosc
            );

            glutSolidCube(1.0);
            break;

        case TypObiektu::Swinia:
            glColor3f(0.35f, 0.82f, 0.22f);

            glutSolidSphere(
                obiekt.szerokosc * 0.5f,
                28, 20
            );
            break;

        case TypObiektu::Ptak:
            glColor3f(0.90f, 0.12f, 0.10f);

            glutSolidSphere(
                obiekt.szerokosc * 0.5f,
                28, 20
            );
            break;
        }


        glPopMatrix();
    }
}

void Renderer::rysujProce(
    const PxVec3& pozycjaPtaka,
    bool pokazGume)
{
    glDisable(GL_LIGHTING);

    glLineWidth(7.0f);
    glColor3f(0.42f, 0.22f, 0.08f);

    glBegin(GL_LINES);

    // Trzon
    glVertex3f(-12.0f, 0.0f, 0.3f);
    glVertex3f(-12.0f, 2.2f, 0.3f);

    // Ramiona
    glVertex3f(-12.0f, 2.0f, 0.3f);
    glVertex3f(-12.7f, 3.0f, 0.3f);

    glVertex3f(-12.0f, 2.0f, 0.3f);
    glVertex3f(-11.3f, 3.0f, 0.3f);

    glEnd();

    if (pokazGume)
    {
        glLineWidth(2.0f);
        glColor3f(0.18f, 0.12f, 0.08f);

        glBegin(GL_LINES);

        glVertex3f(-12.7f, 3.0f, 0.5f);
        glVertex3f(
            pozycjaPtaka.x,
            pozycjaPtaka.y,
            0.6f
        );

        glVertex3f(-11.3f, 3.0f, 0.5f);
        glVertex3f(
            pozycjaPtaka.x,
            pozycjaPtaka.y,
            0.6f
        );

        glEnd();
    }

    glLineWidth(1.0f);
}

void Renderer::rysujPtaka(
    const PxVec3& pozycja)
{
    glPushMatrix();

    // Przed wystrzalem ptak jest jedynie
    // rysowany; nie ma jeszcze aktora PhysX.
    glTranslatef(
        pozycja.x,
        pozycja.y,
        0.7f
    );

    glColor3f(0.90f, 0.12f, 0.10f);
    glutSolidSphere(0.48, 28, 20);

    glPopMatrix();
}

void Renderer::rysujTrajektorie(
    const PxVec3& pozycja)
{
    // Kierunek wystrzalu jest przeciwny
    // do kierunku naciagania procy.
    PxVec3 kierunek =
        POZYCJA_PROCY - pozycja;

    PxVec3 predkosc =
        kierunek * SILA_WYSTRZALU;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glColor3f(1.0f, 0.95f, 0.45f);
    glPointSize(5.0f);

    glBegin(GL_POINTS);

    // Wzor:
    // x(t) = x0 + vx * t
    // y(t) = y0 + vy * t - g*t*t/2
    for (int i = 1; i <= 35; ++i)
    {
        float czas = i * 0.055f;

        float x =
            pozycja.x + predkosc.x * czas;

        float y =
            pozycja.y +
            predkosc.y * czas -
            0.5f * GRAWITACJA * czas * czas;

        if (y < 0.48f)
            break;

        glVertex3f(x, y, 1.0f);
    }

    glEnd();

    glPointSize(1.0f);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::rysujTekst(
    float x,
    float y,
    const char* napis)
{
    glRasterPos2f(x, y);

    while (*napis)
    {
        glutBitmapCharacter(
            GLUT_BITMAP_8_BY_13,
            *napis
        );

        ++napis;
    }
}


void Renderer::rysujInterfejs(
    const SilnikFizyki& silnik,
    bool pauza,
    int pozostalePtaki,
    int punkty,
    const char* stanGry)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    gluOrtho2D(
        0,
        szerokoscOkna,
        wysokoscOkna,
        0
    );

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glColor3f(1.0f, 1.0f, 1.0f);

    rysujTekst(
        20, 25,
        "ANGRY BIRDS - ETAP 4"
    );

    rysujTekst(
        20, 48,
        silnik.czyGpuAktywne()
        ? "PhysX GPU: WLACZONE"
        : "PhysX GPU: WYLACZONE"
    );

    std::ostringstream napis;

    napis << "Pozostale ptaki: "
        << pozostalePtaki << " / 5";

    rysujTekst(
        20, 70,
        napis.str().c_str()
    );

    napis.str("");
    napis.clear();

    napis << "Pozostale swinie: "
        << silnik.pobierzLiczbeSwin();

    rysujTekst(
        20, 92,
        napis.str().c_str()
    );

    napis.str("");
    napis.clear();

    napis << "Punkty: " << punkty;

    rysujTekst(
        20, 114,
        napis.str().c_str()
    );

    rysujTekst(
        20, 140,
        "MYSZ - naciagnij i pusc"
    );

    rysujTekst(
        20, 162,
        "P - pauza | R - reset | ESC - wyjscie"
    );

    // Napis informujacy o stanie poziomu.
    if (stanGry)
    {
        glColor3f(1.0f, 0.95f, 0.3f);

        rysujTekst(
            szerokoscOkna * 0.5f - 130.0f,
            40.0f,
            stanGry
        );
    }

    if (pauza)
    {
        glColor3f(1.0f, 0.85f, 0.2f);

        rysujTekst(
            szerokoscOkna * 0.5f - 25.0f,
            wysokoscOkna * 0.5f,
            "PAUZA"
        );
    }

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}


void Renderer::rysuj(
    const SilnikFizyki& silnik,
    bool pauza,
    bool moznaStrzelac,
    bool naciaganie,
    const PxVec3& pozycjaPtaka,
    int pozostalePtaki,
    int punkty,
    const char* stanGry)
{
    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );

    ustawKamere();

    GLfloat pozycjaSwiatla[] = {
        -5.0f, 14.0f, 12.0f, 1.0f
    };

    GLfloat swiatloOtoczenia[] = {
        0.50f, 0.50f, 0.50f, 1.0f
    };

    glLightfv(
        GL_LIGHT0,
        GL_POSITION,
        pozycjaSwiatla
    );

    glLightModelfv(
        GL_LIGHT_MODEL_AMBIENT,
        swiatloOtoczenia
    );

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    glColorMaterial(
        GL_FRONT_AND_BACK,
        GL_AMBIENT_AND_DIFFUSE
    );

    // Obiekty sterowane przez PhysX.
    rysujObiekty(silnik);

    glDisable(GL_LIGHTING);

    // Proca jest zawsze widoczna.
    rysujProce(
        pozycjaPtaka,
        moznaStrzelac
    );

    // Niewystrzelony ptak jest rysowany
    // w procy, ale jeszcze nie w PhysX.
    if (moznaStrzelac)
    {
        rysujPtaka(pozycjaPtaka);
    }

    if (naciaganie)
    {
        rysujTrajektorie(pozycjaPtaka);
    }

    rysujEtykietyObiektow(silnik);

    rysujInterfejs(
        silnik,
        pauza,
        pozostalePtaki,
        punkty,
        stanGry
    );

    glutSwapBuffers();
}






void Renderer::rysujEtykietyObiektow(
    const SilnikFizyki& silnik)
{
    // Pobieramy aktualne macierze kamery OpenGL.
    GLdouble macierzModelu[16];
    GLdouble macierzProjekcji[16];
    GLint obszarWidoku[4];

    glGetDoublev(
        GL_MODELVIEW_MATRIX,
        macierzModelu
    );

    glGetDoublev(
        GL_PROJECTION_MATRIX,
        macierzProjekcji
    );

    glGetIntegerv(
        GL_VIEWPORT,
        obszarWidoku
    );

    // Przechodzimy do wspolrzednych ekranu.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    gluOrtho2D(
        0,
        szerokoscOkna,
        wysokoscOkna,
        0
    );

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    for (const ObiektSceny& obiekt :
        silnik.pobierzObiekty())
    {
        if (!obiekt.aktor)
            continue;

        // Nie wyswietlamy HP podloza i ptakow.
        if (obiekt.typ == TypObiektu::Podloze ||
            obiekt.typ == TypObiektu::Ptak)
        {
            continue;
        }

        // Odczyt rzeczywistej pozycji PhysX.
        const physx::PxTransform pozycja =
            obiekt.aktor->getGlobalPose();

        // Pozycja napisu nieco nad obiektem.
        const double x = pozycja.p.x;
        const double y =
            pozycja.p.y +
            obiekt.wysokosc * 0.5f + 0.35f;

        const double z = pozycja.p.z;

        GLdouble ekranX;
        GLdouble ekranY;
        GLdouble ekranZ;

        // Zamiana pozycji 3D na piksele.
        const GLint poprawne = gluProject(
            x, y, z,
            macierzModelu,
            macierzProjekcji,
            obszarWidoku,
            &ekranX,
            &ekranY,
            &ekranZ
        );

        if (poprawne != GL_TRUE ||
            ekranZ < 0.0 || ekranZ > 1.0)
        {
            continue;
        }

        const float px =
            static_cast<float>(ekranX);

        const float py =
            static_cast<float>(
                obszarWidoku[3] - ekranY
                );

        // Pomijamy napisy poza oknem.
        if (px < 0.0f ||
            px > szerokoscOkna ||
            py < 0.0f ||
            py > wysokoscOkna)
        {
            continue;
        }

        const char* nazwaMaterialu = "";

        switch (obiekt.typ)
        {
        case TypObiektu::Drewno:
            nazwaMaterialu = "DREWNO";
            break;

        case TypObiektu::Szklo:
            nazwaMaterialu = "SZKLO";
            break;

        case TypObiektu::Kamien:
            nazwaMaterialu = "KAMIEN";
            break;

        case TypObiektu::Swinia:
            nazwaMaterialu = "SWINIA";
            break;

        default:
            continue;
        }

        // Budujemy napis z aktualnym HP.
        std::ostringstream napisHP;

        napisHP
            << nazwaMaterialu
            << ": "
            << static_cast<int>(
                std::ceil(obiekt.wytrzymalosc)
                )
            << " / "
            << static_cast<int>(
                std::ceil(obiekt.maksymalnaWytrzymalosc)
                )
            << " HP";

        const std::string tekstHP =
            napisHP.str();

        // Centrowanie napisu nad obiektem.
        const float szerokoscTekstu =
            static_cast<float>(
                tekstHP.length() * 8
                );

        const float tekstX =
            px - szerokoscTekstu * 0.5f;

        // Kolor informacji o zdrowiu.
        if (obiekt.wytrzymalosc <=
            obiekt.maksymalnaWytrzymalosc * 0.25f)
        {
            glColor3f(1.0f, 0.25f, 0.25f);
        }
        else
        {
            glColor3f(1.0f, 1.0f, 1.0f);
        }

        rysujTekst(
            tekstX,
            py,
            tekstHP.c_str()
        );

        // Ostatnie obrazenia wyswietlamy
        // tylko przez okreslony czas.
        if (obiekt.czasPokazaniaObrazen > 0.0f &&
            obiekt.ostatnieObrazenia > 0.0f)
        {
            const char* nazwaSprawcy = "";

            switch (obiekt.ostatniSprawca)
            {
            case TypObiektu::Drewno:
                nazwaSprawcy = "DREWNO";
                break;

            case TypObiektu::Szklo:
                nazwaSprawcy = "SZKLO";
                break;

            case TypObiektu::Kamien:
                nazwaSprawcy = "KAMIEN";
                break;

            case TypObiektu::Ptak:
                nazwaSprawcy = "PTAK";
                break;

            case TypObiektu::Podloze:
                nazwaSprawcy = "PODLOZE";
                break;

            default:
                nazwaSprawcy = "OBIEKT";
                break;
            }

            std::ostringstream napisObrazen;

            napisObrazen
                << "-"
                << std::fixed
                << std::setprecision(1)
                << obiekt.ostatnieObrazenia
                << " HP ("
                << nazwaSprawcy
                << ")";

            const std::string tekstObrazen =
                napisObrazen.str();

            const float szerokoscObrazen =
                static_cast<float>(
                    tekstObrazen.length() * 8
                    );

            glColor3f(1.0f, 0.20f, 0.20f);

            rysujTekst(
                px - szerokoscObrazen * 0.5f,
                py - 18.0f,
                tekstObrazen.c_str()
            );
        }
    }

    // Przywracamy poprzednie ustawienia.
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

