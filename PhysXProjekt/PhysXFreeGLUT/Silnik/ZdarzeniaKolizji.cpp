
#include "ZdarzeniaKolizji.h"

#include <algorithm>
#include <mutex>
#include <utility>

using namespace physx;

void ZdarzeniaKolizji::onContact(
    const PxContactPairHeader& naglowek,
    const PxContactPair* pary,
    PxU32 liczbaPar)
{
    // Nie modyfikujemy tutaj sceny PhysX.
    // Zbieramy tylko informacje o uderzeniach.

    for (PxU32 i = 0; i < liczbaPar; ++i)
    {
        const PxContactPair& para = pary[i];

        // Obrazenia liczymy tylko przy rozpoczeciu
        // nowego kontaktu, a nie przy jego trwaniu.
        if (!para.events.isSet(
            PxPairFlag::eNOTIFY_TOUCH_FOUND))
        {
            continue;
        }

        if (para.flags.isSet(physx::PxContactPairFlag::eREMOVED_SHAPE_0) ||
            para.flags.isSet(physx::PxContactPairFlag::eREMOVED_SHAPE_1))
        {
            continue;
        }

        if (para.contactCount == 0)
            continue;

        constexpr PxU32 MAKS_KONTAKTOW = 64;

        PxContactPairPoint kontakty[MAKS_KONTAKTOW];

        const PxU32 liczbaKontaktow =
            para.extractContacts(
                kontakty,
                MAKS_KONTAKTOW
            );

        float sumaImpulsow = 0.0f;

        for (PxU32 j = 0; j < liczbaKontaktow; ++j)
        {
            // Impuls pochodzi z rozwiazania
            // kontaktu przez silnik PhysX.
            sumaImpulsow +=
                kontakty[j].impulse.magnitude();
        }

        if (sumaImpulsow <= 0.0f)
            continue;

        ZdarzenieZderzenia zdarzenie;

        zdarzenie.aktorA =
            static_cast<physx::PxRigidActor*>(
                naglowek.actors[0]
                );

        zdarzenie.aktorB =
            static_cast<physx::PxRigidActor*>(
                naglowek.actors[1]
                );

        zdarzenie.impuls = sumaImpulsow;


        std::lock_guard<std::mutex> blokada(
            mutexZderzen
        );

        zderzenia.push_back(zdarzenie);
    }
}

std::vector<ZdarzenieZderzenia>
ZdarzeniaKolizji::pobierzZderzenia()
{
    std::lock_guard<std::mutex> blokada(
        mutexZderzen
    );

    std::vector<ZdarzenieZderzenia> wynik;
    wynik.swap(zderzenia);

    return wynik;
}

void ZdarzeniaKolizji::wyczysc()
{
    std::lock_guard<std::mutex> blokada(
        mutexZderzen
    );

    zderzenia.clear();
}

void ZdarzeniaKolizji::onConstraintBreak(
    PxConstraintInfo*,
    PxU32)
{
}

void ZdarzeniaKolizji::onWake(
    PxActor**,
    PxU32)
{
}

void ZdarzeniaKolizji::onSleep(
    PxActor**,
    PxU32)
{
}

void ZdarzeniaKolizji::onTrigger(
    PxTriggerPair*,
    PxU32)
{
}

void ZdarzeniaKolizji::onAdvance(
    const PxRigidBody* const*,
    const PxTransform*,
    const PxU32)
{
}
