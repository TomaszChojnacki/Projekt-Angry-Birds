
#pragma once

#include "PxPhysicsAPI.h"
#include "gpu/PxGpu.h"
#include "cudamanager/PxCudaContextManager.h"

#include "ZdarzeniaKolizji.h"

#include <vector>

// Rodzaje obiektow gry.
enum class TypObiektu
{
    Podloze,
    Drewno,
    Szklo,
    Kamien,
    Swinia,
    Ptak
};

// Dane pojedynczego obiektu sceny.

struct ObiektSceny
{
    physx::PxRigidActor* aktor = nullptr;

    TypObiektu typ = TypObiektu::Drewno;

    float szerokosc = 1.0f;
    float wysokosc = 1.0f;
    float glebokosc = 1.0f;

    // Aktualna i poczatkowa wytrzymalosc.
    float wytrzymalosc = 0.0f;
    float maksymalnaWytrzymalosc = 0.0f;

    // Parametry obrazen od uderzenia.
    float progUderzenia = 0.0f;
    float mnoznikObrazen = 0.0f;

    // Ochrona przed wielokrotnym naliczaniem
    // obrazen w bardzo krotkim czasie.
    float czasOchrony = 0.0f;

    bool doUsuniecia = false;

    // Dane wyswietlane nad obiektem.
    float ostatnieObrazenia = 0.0f;
    float czasPokazaniaObrazen = 0.0f;
    // Czas pozostaly do usuniecia zniszczonego obiektu.
    float czasDoUsuniecia = 0.0f;

    TypObiektu ostatniSprawca =
        TypObiektu::Podloze;
};


class SilnikFizyki
{
public:
    SilnikFizyki() = default;
    ~SilnikFizyki();

    bool inicjalizuj();

    void aktualizuj(float krokCzasowy);
    void wyczysc();

    bool czyGpuAktywne() const;

    const std::vector<ObiektSceny>&
        pobierzObiekty() const;

    // Tworzenie fizycznego ptaka przy strzale.
    physx::PxRigidDynamic* wystrzelPtaka(
        const physx::PxVec3& pozycja,
        const physx::PxVec3& predkosc
    );
    // Usuwa wystrzelonego ptaka z fizyki i renderowania.
    bool usunPtaka(physx::PxRigidDynamic* ptak);

    // Informacje potrzebne w kolejnych etapach.
    int pobierzLiczbeSwin() const;
    int pobierzWyeliminowaneSwinie() const;

private:
    physx::PxDefaultAllocator alokator;
    physx::PxDefaultErrorCallback obslugaBledow;

    physx::PxFoundation* fundament = nullptr;
    physx::PxPhysics* fizyka = nullptr;
    physx::PxScene* scena = nullptr;

    physx::PxDefaultCpuDispatcher* dyspozytor =
        nullptr;

    physx::PxCudaContextManager* menedzerCuda =
        nullptr;

    physx::PxMaterial* materialPodloza = nullptr;
    physx::PxMaterial* materialDrewna = nullptr;
    physx::PxMaterial* materialSzkla = nullptr;
    physx::PxMaterial* materialKamienia = nullptr;
    physx::PxMaterial* materialSwini = nullptr;
    physx::PxMaterial* materialPtaka = nullptr;

    ZdarzeniaKolizji rejestratorKolizji;

    std::vector<ObiektSceny> obiekty;

    bool gpuAktywne = false;

    int wyeliminowaneSwinie = 0;

    bool utworzPodloze();
    bool utworzBudowle();
    bool utworzSwinie();

    bool dodajStatycznyProstopadloscian(
        float x,
        float y,
        float szerokosc,
        float wysokosc,
        TypObiektu typ
    );

    bool dodajDynamicznyProstopadloscian(
        float x,
        float y,
        float szerokosc,
        float wysokosc,
        TypObiektu typ
    );

    // System zderzen i uszkodzen.
    void przetworzZderzenia();
    void zadajObrazenia(
        physx::PxRigidActor* cel,
        physx::PxRigidActor* sprawca,
        float impuls
    );

    void usunZniszczoneObiekty();
};
