
#pragma once

#include "PxPhysicsAPI.h"

#include <mutex>
#include <vector>

// Informacje o pojedynczym zderzeniu.
struct ZdarzenieZderzenia
{
    physx::PxRigidActor* aktorA = nullptr;
    physx::PxRigidActor* aktorB = nullptr;

    // Suma wartosci impulsow kontaktowych [N*s].
    float impuls = 0.0f;
};

// Odbieranie zdarzen z silnika PhysX.
class ZdarzeniaKolizji
    : public physx::PxSimulationEventCallback
{
public:
    void onContact(
        const physx::PxContactPairHeader& naglowek,
        const physx::PxContactPair* pary,
        physx::PxU32 liczbaPar
    ) override;

    // Metody wymagane przez interfejs PhysX.
    void onConstraintBreak(
        physx::PxConstraintInfo*,
        physx::PxU32
    ) override;

    void onWake(
        physx::PxActor**,
        physx::PxU32
    ) override;

    void onSleep(
        physx::PxActor**,
        physx::PxU32
    ) override;

    void onTrigger(
        physx::PxTriggerPair*,
        physx::PxU32
    ) override;

    void onAdvance(
        const physx::PxRigidBody* const*,
        const physx::PxTransform*,
        const physx::PxU32
    ) override;

    // Pobieranie kontaktow po kroku symulacji.
    std::vector<ZdarzenieZderzenia>
        pobierzZderzenia();

    void wyczysc();

private:
    std::vector<ZdarzenieZderzenia> zderzenia;

    // Chroni liste przed rownoleglymi callbackami.
    std::mutex mutexZderzen;
};
