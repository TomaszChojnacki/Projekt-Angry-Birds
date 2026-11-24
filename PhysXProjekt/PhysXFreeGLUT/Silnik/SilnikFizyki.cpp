
#include "SilnikFizyki.h"

#include <algorithm>
#include <iostream>


using namespace physx;

namespace
{
    constexpr float GRAWITACJA = 9.81f;
    constexpr float GLEBOKOSC = 1.0f;

    constexpr float CZAS_OCHRONY = 0.15f;

    // Wlasny filtr kontaktow.
    // PhysX nadal odpowiada za rozwiazywanie kolizji.
    PxFilterFlags filtrKontaktow(
        PxFilterObjectAttributes atrybutyA,
        PxFilterData,
        PxFilterObjectAttributes atrybutyB,
        PxFilterData,
        PxPairFlags& flagiPary,
        const void*,
        PxU32)
    {
        if (PxFilterObjectIsTrigger(atrybutyA) ||
            PxFilterObjectIsTrigger(atrybutyB))
        {
            flagiPary =
                PxPairFlag::eTRIGGER_DEFAULT;

            return PxFilterFlag::eDEFAULT;
        }

        flagiPary = PxPairFlag::eCONTACT_DEFAULT;


        flagiPary |=
            PxPairFlag::eNOTIFY_TOUCH_FOUND;

        // Potrzebujemy impulsow z punktow kontaktu.
        flagiPary |=
            PxPairFlag::eNOTIFY_CONTACT_POINTS;

        return PxFilterFlag::eDEFAULT;
    }
}

// ---------------------------------------------
// INICJALIZACJA
// ---------------------------------------------

SilnikFizyki::~SilnikFizyki()
{
    wyczysc();
}

bool SilnikFizyki::inicjalizuj()
{
    wyeliminowaneSwinie = 0;
    rejestratorKolizji.wyczysc();

    fundament = PxCreateFoundation(
        PX_PHYSICS_VERSION,
        alokator,
        obslugaBledow
    );

    if (!fundament)
        return false;

    fizyka = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *fundament,
        PxTolerancesScale()
    );

    if (!fizyka)
        return false;

    // Inicjalizacja NVIDIA CUDA.
    PxCudaContextManagerDesc opisCuda;

    menedzerCuda = PxCreateCudaContextManager(
        *fundament,
        opisCuda
    );

    if (!menedzerCuda ||
        !menedzerCuda->contextIsValid())
    {
        std::cerr << "Blad inicjalizacji CUDA.\n";
        return false;
    }

    std::cout << "GPU: "
        << menedzerCuda->getDeviceName()
        << "\n";

    // Konfiguracja sceny PhysX.
    PxSceneDesc opisSceny(
        fizyka->getTolerancesScale()
    );

    opisSceny.gravity = PxVec3(
        0.0f,
        -GRAWITACJA,
        0.0f
    );

    dyspozytor = PxDefaultCpuDispatcherCreate(2);

    if (!dyspozytor)
        return false;

    opisSceny.cpuDispatcher = dyspozytor;

    // Nasz filtr przekazuje informacje o kontaktach.
    opisSceny.filterShader = filtrKontaktow;

    opisSceny.simulationEventCallback =
        &rejestratorKolizji;

    opisSceny.cudaContextManager = menedzerCuda;

    opisSceny.flags |=
        PxSceneFlag::eENABLE_GPU_DYNAMICS;

    opisSceny.flags |=
        PxSceneFlag::eENABLE_PCM;

    opisSceny.broadPhaseType =
        PxBroadPhaseType::eGPU;

    opisSceny.bounceThresholdVelocity = 1.0f;

    scena = fizyka->createScene(opisSceny);

    if (!scena)
        return false;

    gpuAktywne =
        scena->getFlags().isSet(
            PxSceneFlag::eENABLE_GPU_DYNAMICS
        ) &&
        scena->getBroadPhaseType() ==
        PxBroadPhaseType::eGPU;

    // Materialy:
    // createMaterial(tarcieStatyczne,
    //                tarcieDynamiczne,
    //                restytucja)

    materialPodloza = fizyka->createMaterial(
        0.9f, 0.8f, 0.1f
    );

    materialDrewna = fizyka->createMaterial(
        0.7f, 0.6f, 0.1f
    );

    materialSzkla = fizyka->createMaterial(
        0.4f, 0.3f, 0.15f
    );

    materialKamienia = fizyka->createMaterial(
        0.9f, 0.8f, 0.05f
    );

    materialSwini = fizyka->createMaterial(
        0.6f, 0.5f, 0.2f
    );

    materialPtaka = fizyka->createMaterial(
        0.5f, 0.4f, 0.25f
    );

    if (!materialPodloza ||
        !materialDrewna ||
        !materialSzkla ||
        !materialKamienia ||
        !materialSwini ||
        !materialPtaka)
    {
        return false;
    }

    if (!utworzPodloze())
        return false;

    if (!utworzBudowle())
        return false;

    if (!utworzSwinie())
        return false;

    std::cout << "PhysX GPU: "
        << (gpuAktywne ? "WLACZONE" : "WYLACZONE")
        << "\n";

    std::cout << "Liczba obiektow: "
        << obiekty.size() << "\n";

    return true;
}

// ---------------------------------------------
// TWORZENIE OBIEKTOW
// ---------------------------------------------

bool SilnikFizyki::dodajStatycznyProstopadloscian(
    float x,
    float y,
    float szerokosc,
    float wysokosc,
    TypObiektu typ)
{
    PxRigidStatic* aktor =
        fizyka->createRigidStatic(
            PxTransform(PxVec3(x, y, 0.0f))
        );

    if (!aktor)
        return false;

    PxBoxGeometry geometria(
        szerokosc * 0.5f,
        wysokosc * 0.5f,
        GLEBOKOSC * 0.5f
    );

    PxShape* ksztalt =
        PxRigidActorExt::createExclusiveShape(
            *aktor,
            geometria,
            *materialPodloza
        );

    if (!ksztalt)
    {
        aktor->release();
        return false;
    }

    scena->addActor(*aktor);

    ObiektSceny obiekt;

    obiekt.aktor = aktor;
    obiekt.typ = typ;
    obiekt.szerokosc = szerokosc;
    obiekt.wysokosc = wysokosc;
    obiekt.glebokosc = GLEBOKOSC;

    // Podloze jest niezniszczalne.
    obiekt.wytrzymalosc = 0.0f;
    obiekt.progUderzenia = 0.0f;

    obiekty.push_back(obiekt);

    return true;
}

bool SilnikFizyki::dodajDynamicznyProstopadloscian(
    float x,
    float y,
    float szerokosc,
    float wysokosc,
    TypObiektu typ)
{
    PxMaterial* material = nullptr;

    float gestosc = 1.0f;
    float wytrzymalosc = 0.0f;
    float progUderzenia = 0.0f;
    float mnoznikObrazen = 0.0f;

    // Ustawienia materialow.



    switch (typ)
    {
    case TypObiektu::Drewno:
        material = materialDrewna;
        gestosc = 2.0f;

        wytrzymalosc = 100.0f;
        progUderzenia = 3.0f;
        mnoznikObrazen = 4.0f;
        break;

    case TypObiektu::Szklo:
        material = materialSzkla;
        gestosc = 1.0f;

        wytrzymalosc = 40.0f;
        progUderzenia = 1.5f;
        mnoznikObrazen = 6.0f;
        break;

    case TypObiektu::Kamien:
        material = materialKamienia;
        gestosc = 5.0f;

        wytrzymalosc = 220.0f;
        progUderzenia = 9.0f;
        mnoznikObrazen = 1.5f;
        break;

    default:
        return false;
    }



    PxBoxGeometry geometria(
        szerokosc * 0.5f,
        wysokosc * 0.5f,
        GLEBOKOSC * 0.5f
    );

    PxRigidDynamic* aktor = PxCreateDynamic(
        *fizyka,
        PxTransform(PxVec3(x, y, 0.0f)),
        geometria,
        *material,
        gestosc
    );

    if (!aktor)
        return false;

    // Ruch w plaszczyznie X/Y.
    aktor->setRigidDynamicLockFlags(
        PxRigidDynamicLockFlag::eLOCK_LINEAR_Z |
        PxRigidDynamicLockFlag::eLOCK_ANGULAR_X |
        PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y
    );

    aktor->setLinearDamping(0.12f);
    aktor->setAngularDamping(0.20f);

    aktor->setSolverIterationCounts(8, 2);

    scena->addActor(*aktor);

    ObiektSceny obiekt;

    obiekt.aktor = aktor;
    obiekt.typ = typ;

    obiekt.szerokosc = szerokosc;
    obiekt.wysokosc = wysokosc;
    obiekt.glebokosc = GLEBOKOSC;

    obiekt.wytrzymalosc = wytrzymalosc;
    obiekt.maksymalnaWytrzymalosc = wytrzymalosc;

    obiekt.progUderzenia = progUderzenia;
    obiekt.mnoznikObrazen = mnoznikObrazen;

    obiekty.push_back(obiekt);

    return true;
}

bool SilnikFizyki::utworzPodloze()
{
    return dodajStatycznyProstopadloscian(
        0.0f,
        -0.5f,
        36.0f,
        1.0f,
        TypObiektu::Podloze
    );
}

bool SilnikFizyki::utworzBudowle()
{
    // DREWNO: dolne podpory.
    if (!dodajDynamicznyProstopadloscian(
        8.0f, 1.2f,
        0.8f, 2.4f,
        TypObiektu::Drewno))
        return false;

    if (!dodajDynamicznyProstopadloscian(
        12.0f, 1.2f,
        0.8f, 2.4f,
        TypObiektu::Drewno))
        return false;

    // DREWNO: srodkowa belka.
    if (!dodajDynamicznyProstopadloscian(
        10.0f, 2.7f,
        5.2f, 0.6f,
        TypObiektu::Drewno))
        return false;

    // DREWNO: gorne podpory.
    if (!dodajDynamicznyProstopadloscian(
        8.5f, 3.8f,
        0.8f, 1.6f,
        TypObiektu::Drewno))
        return false;

    if (!dodajDynamicznyProstopadloscian(
        11.5f, 3.8f,
        0.8f, 1.6f,
        TypObiektu::Drewno))
        return false;

    // DREWNO: gorna belka.
    if (!dodajDynamicznyProstopadloscian(
        10.0f, 4.9f,
        4.2f, 0.6f,
        TypObiektu::Drewno))
        return false;

    // SZKLO: elementy na szczycie konstrukcji.
    if (!dodajDynamicznyProstopadloscian(
        9.4f, 5.65f,
        0.7f, 0.9f,
        TypObiektu::Szklo))
        return false;

    if (!dodajDynamicznyProstopadloscian(
        10.6f, 5.65f,
        0.7f, 0.9f,
        TypObiektu::Szklo))
        return false;

    // KAMIEN: osobny blok przy budowli.
    if (!dodajDynamicznyProstopadloscian(
        14.0f, 0.75f,
        1.5f, 1.5f,
        TypObiektu::Kamien))
        return false;

    return true;
}

bool SilnikFizyki::utworzSwinie()
{
    constexpr float PROMIEN = 0.6f;

    PxRigidDynamic* aktor = PxCreateDynamic(
        *fizyka,
        PxTransform(
            PxVec3(10.0f, PROMIEN, 0.0f)
        ),
        PxSphereGeometry(PROMIEN),
        *materialSwini,
        1.0f
    );

    if (!aktor)
        return false;

    aktor->setRigidDynamicLockFlags(
        PxRigidDynamicLockFlag::eLOCK_LINEAR_Z |
        PxRigidDynamicLockFlag::eLOCK_ANGULAR_X |
        PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y
    );

    aktor->setLinearDamping(0.3f);
    aktor->setAngularDamping(0.3f);

    aktor->setSolverIterationCounts(8, 2);

    scena->addActor(*aktor);

    ObiektSceny obiekt;

    obiekt.aktor = aktor;
    obiekt.typ = TypObiektu::Swinia;

    obiekt.szerokosc = PROMIEN * 2.0f;
    obiekt.wysokosc = PROMIEN * 2.0f;
    obiekt.glebokosc = PROMIEN * 2.0f;


    // Wytrzymalosc swini.
    obiekt.wytrzymalosc = 95.0f;
    obiekt.maksymalnaWytrzymalosc = 95.0f;

    // Minimalny impuls powodujacy obrazenia.
    obiekt.progUderzenia = 1.5f;

    // Podstawowy mnoznik obrazen.
    obiekt.mnoznikObrazen = 3.4f;


    obiekty.push_back(obiekt);

    return true;
}

PxRigidDynamic* SilnikFizyki::wystrzelPtaka(
    const PxVec3& pozycja,
    const PxVec3& predkosc)
{
    if (!fizyka || !scena || !materialPtaka)
        return nullptr;

    constexpr float PROMIEN = 0.48f;
    constexpr float GESTOSC = 2.0f;

    PxRigidDynamic* aktor = PxCreateDynamic(
        *fizyka,
        PxTransform(pozycja),
        PxSphereGeometry(PROMIEN),
        *materialPtaka,
        GESTOSC
    );

    if (!aktor)
        return nullptr;

    aktor->setRigidDynamicLockFlags(
        PxRigidDynamicLockFlag::eLOCK_LINEAR_Z |
        PxRigidDynamicLockFlag::eLOCK_ANGULAR_X |
        PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y
    );

    aktor->setLinearDamping(0.02f);
    aktor->setAngularDamping(0.10f);

    aktor->setSolverIterationCounts(8, 2);

    scena->addActor(*aktor);

    aktor->setLinearVelocity(predkosc);
    aktor->wakeUp();

    ObiektSceny obiekt;

    obiekt.aktor = aktor;
    obiekt.typ = TypObiektu::Ptak;

    obiekt.szerokosc = PROMIEN * 2.0f;
    obiekt.wysokosc = PROMIEN * 2.0f;
    obiekt.glebokosc = PROMIEN * 2.0f;

    // W tym etapie ptaki nie maja systemu HP.
    obiekty.push_back(obiekt);

    std::cout << "Wystrzelono ptaka.\n";

    return aktor;
}

// ---------------------------------------------
// SYSTEM OBRAZEN
// ---------------------------------------------



void SilnikFizyki::zadajObrazenia(
    PxRigidActor* cel,
    PxRigidActor* sprawca,
    float impuls)
{
    if (!cel || !sprawca)
        return;

    // Szukamy obiektu, ktory otrzymal uderzenie.
    ObiektSceny* obiektCelu = nullptr;

    // Sprawdzamy, jaki obiekt spowodowal kolizje.
    TypObiektu typSprawcy = TypObiektu::Podloze;

    for (ObiektSceny& obiekt : obiekty)
    {
        if (obiekt.aktor == cel)
            obiektCelu = &obiekt;

        if (obiekt.aktor == sprawca)
            typSprawcy = obiekt.typ;
    }

    if (!obiektCelu)
        return;

    // Podloze i ptaki sa na razie niezniszczalne.
    if (obiektCelu->typ == TypObiektu::Podloze ||
        obiektCelu->typ == TypObiektu::Ptak)
    {
        return;
    }

    if (obiektCelu->doUsuniecia)
        return;

    // Nie liczymy kolejnego uderzenia
    // podczas krotkiego okresu ochronnego.
    if (obiektCelu->czasOchrony > 0.0f)
        return;

    // Uderzenia ponizej progu nie uszkadzaja.
    if (impuls <= obiektCelu->progUderzenia)
        return;

    // Podstawowy wzor na obrazenia:
    // D = (J - J_min) * k
    const float nadwyzkaImpulsu =
        impuls - obiektCelu->progUderzenia;

    float obrazenia =
        nadwyzkaImpulsu *
        obiektCelu->mnoznikObrazen;

    // Jezeli uderzono swinie, dodatkowo
    // uwzgledniamy rodzaj obiektu uderzajacego.
    if (obiektCelu->typ == TypObiektu::Swinia)
    {
        float mnoznikMaterialu = 1.0f;


        switch (typSprawcy)
        {
        case TypObiektu::Ptak:
            // Bezposrednie trafienie ptakiem.
            mnoznikMaterialu = 1.6f;
            break;

        case TypObiektu::Drewno:
            // Spadajaca drewniana belka.
            mnoznikMaterialu = 1.4f;
            break;

        case TypObiektu::Szklo:
            // Lekkie szklo zadaje mniejsze obrazenia.
            mnoznikMaterialu = 0.8f;
            break;

        case TypObiektu::Kamien:
            // Ciezki kamien jest bardzo niebezpieczny.
            mnoznikMaterialu = 3.0f;
            break;

        case TypObiektu::Podloze:
            // Zwykle dotkniecie podloza nie zadaje obrazen.
            mnoznikMaterialu = 0.0f;
            break;

        default:
            mnoznikMaterialu = 1.0f;
            break;
        }


        obrazenia *= mnoznikMaterialu;
    }

    if (obrazenia <= 0.0f)
        return;

    // Nie mozemy zabrac wiecej HP,
    // niz obiektowi faktycznie pozostalo.
    const float rzeczywisteObrazenia =
        std::min(
            obrazenia,
            obiektCelu->wytrzymalosc
        );

    obiektCelu->wytrzymalosc =
        std::max(
            0.0f,
            obiektCelu->wytrzymalosc -
            rzeczywisteObrazenia
        );

    // Zapisujemy ostatnie obrazenia do HUD.
    obiektCelu->ostatnieObrazenia =
        rzeczywisteObrazenia;

    obiektCelu->ostatniSprawca =
        typSprawcy;

    // Napis bedzie widoczny przez 1.8 sekundy.
    obiektCelu->czasPokazaniaObrazen = 1.8f;

    // Ochrona przed seri¹ zdarzen kontaktowych.
    obiektCelu->czasOchrony = 0.20f;

    std::cout
        << "Impuls: " << impuls
        << " N*s | Obrazenia: "
        << rzeczywisteObrazenia
        << " | HP: "
        << obiektCelu->wytrzymalosc
        << "/"
        << obiektCelu->maksymalnaWytrzymalosc
        << "\n";

    if (obiektCelu->wytrzymalosc <= 0.0f)
    {
        obiektCelu->doUsuniecia = true;
        obiektCelu->czasDoUsuniecia = 0.9f;
    }
}




void SilnikFizyki::przetworzZderzenia()
{
    const auto zdarzenia =
        rejestratorKolizji.pobierzZderzenia();

    for (const ZdarzenieZderzenia& zdarzenie :
        zdarzenia)
    {
        // Aktor A otrzymuje obrazenia od B.
        zadajObrazenia(
            zdarzenie.aktorA,
            zdarzenie.aktorB,
            zdarzenie.impuls
        );

        // Aktor B otrzymuje obrazenia od A.
        zadajObrazenia(
            zdarzenie.aktorB,
            zdarzenie.aktorA,
            zdarzenie.impuls
        );
    }

    // Usuwanie dopiero po symulacji PhysX.
    usunZniszczoneObiekty();
}



void SilnikFizyki::usunZniszczoneObiekty()
{
    for (ObiektSceny& obiekt : obiekty)
    {
        if (!obiekt.doUsuniecia)
            continue;

        // Dajemy graczowi czas na odczytanie
        // informacji o ostatnich obrazeniach.
        if (obiekt.czasDoUsuniecia > 0.0f)
            continue;

        if (obiekt.typ == TypObiektu::Swinia)
        {
            ++wyeliminowaneSwinie;

            std::cout
                << "SWINIA WYELIMINOWANA!\n";
        }
        else
        {
            std::cout
                << "ELEMENT BUDOWLI ZNISZCZONY!\n";
        }

        if (obiekt.aktor)
        {
            obiekt.aktor->release();
            obiekt.aktor = nullptr;
        }
    }

    obiekty.erase(
        std::remove_if(
            obiekty.begin(),
            obiekty.end(),
            [](const ObiektSceny& obiekt)
            {
                return obiekt.doUsuniecia &&
                    obiekt.czasDoUsuniecia <= 0.0f;
            }
        ),
        obiekty.end()
    );
}


// ---------------------------------------------
// AKTUALIZACJA SYMULACJI
// ---------------------------------------------


void SilnikFizyki::aktualizuj(float krokCzasowy)
{
    if (!scena)
        return;

    for (ObiektSceny& obiekt : obiekty)
    {
        // Ochrona przed kolejnym uderzeniem.
        if (obiekt.czasOchrony > 0.0f)
        {
            obiekt.czasOchrony =
                std::max(
                    0.0f,
                    obiekt.czasOchrony - krokCzasowy
                );
        }

        // Czas pokazywania obrazen nad obiektem.
        if (obiekt.czasPokazaniaObrazen > 0.0f)
        {
            obiekt.czasPokazaniaObrazen =
                std::max(
                    0.0f,
                    obiekt.czasPokazaniaObrazen -
                    krokCzasowy
                );
        }

        // Odliczanie czasu do usuniecia obiektu.
        if (obiekt.doUsuniecia &&
            obiekt.czasDoUsuniecia > 0.0f)
        {
            obiekt.czasDoUsuniecia =
                std::max(
                    0.0f,
                    obiekt.czasDoUsuniecia - krokCzasowy
                );
        }
    }

    scena->simulate(krokCzasowy);
    scena->fetchResults(true);

    przetworzZderzenia();
}


// ---------------------------------------------
// ODCZYT STANU SCENY
// ---------------------------------------------

bool SilnikFizyki::czyGpuAktywne() const
{
    return gpuAktywne;
}

const std::vector<ObiektSceny>&
SilnikFizyki::pobierzObiekty() const
{
    return obiekty;
}

int SilnikFizyki::pobierzLiczbeSwin() const
{
    int liczba = 0;

    for (const ObiektSceny& obiekt : obiekty)
    {
        if (obiekt.typ == TypObiektu::Swinia)
            ++liczba;
    }

    return liczba;
}

int SilnikFizyki::pobierzWyeliminowaneSwinie() const
{
    return wyeliminowaneSwinie;
}

// ---------------------------------------------
// ZWALNIANIE ZASOBOW
// ---------------------------------------------

void SilnikFizyki::wyczysc()
{
    // Usuwamy aktorow przed scena.
    for (ObiektSceny& obiekt : obiekty)
    {
        if (obiekt.aktor)
        {
            obiekt.aktor->release();
            obiekt.aktor = nullptr;
        }
    }

    obiekty.clear();

    if (scena)
    {
        scena->release();
        scena = nullptr;
    }

    rejestratorKolizji.wyczysc();

    if (materialPtaka)
    {
        materialPtaka->release();
        materialPtaka = nullptr;
    }

    if (materialSwini)
    {
        materialSwini->release();
        materialSwini = nullptr;
    }

    if (materialKamienia)
    {
        materialKamienia->release();
        materialKamienia = nullptr;
    }

    if (materialSzkla)
    {
        materialSzkla->release();
        materialSzkla = nullptr;
    }

    if (materialDrewna)
    {
        materialDrewna->release();
        materialDrewna = nullptr;
    }

    if (materialPodloza)
    {
        materialPodloza->release();
        materialPodloza = nullptr;
    }

    if (dyspozytor)
    {
        dyspozytor->release();
        dyspozytor = nullptr;
    }

    if (menedzerCuda)
    {
        menedzerCuda->release();
        menedzerCuda = nullptr;
    }

    if (fizyka)
    {
        fizyka->release();
        fizyka = nullptr;
    }

    if (fundament)
    {
        fundament->release();
        fundament = nullptr;
    }

    gpuAktywne = false;
    wyeliminowaneSwinie = 0;
}






bool SilnikFizyki::usunPtaka(PxRigidDynamic* ptak)
{
    if (!ptak)
        return false;

    for (auto it = obiekty.begin();
        it != obiekty.end();
        ++it)
    {
        // Usuwamy wylacznie wskazanego ptaka.
        if (it->aktor != ptak ||
            it->typ != TypObiektu::Ptak)
        {
            continue;
        }

        // Zwolnienie obiektu fizycznego.
        ptak->release();

        // Usuniecie z listy renderowanych obiektow.
        obiekty.erase(it);

        std::cout << "Usunieto poprzedniego ptaka.\n";

        return true;
    }

    return false;
}
