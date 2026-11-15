
#include <GL/freeglut.h>
#include <GL/glu.h>

#include "PxPhysicsAPI.h"
#include "gpu/PxGpu.h"
#include "cudamanager/PxCudaContextManager.h"

#include <iostream>
#include <vector>
#include <cstdlib>

using namespace physx;

// PhysX
PxDefaultAllocator gAllocator;
PxDefaultErrorCallback gErrorCallback;

PxFoundation* gFoundation = nullptr;
PxPhysics* gPhysics = nullptr;
PxScene* gScene = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;
PxCudaContextManager* gCudaManager = nullptr;
PxMaterial* gMaterial = nullptr;

std::vector<PxRigidDynamic*> gCubes;

bool gSimulationRunning = true;

void printGPUInfo()
{
    std::cout << "\n========== NVIDIA GPU ==========\n";

    if (!gCudaManager || !gCudaManager->contextIsValid())
    {
        std::cout << "CUDA: NIEDOSTEPNA\n";
        return;
    }

    std::cout << "Karta: "
        << gCudaManager->getDeviceName() << "\n";

    std::cout << "Kontekst CUDA: POPRAWNY\n";

    int driver = gCudaManager->getDriverVersion();

    std::cout << "Sterownik CUDA API: "
        << driver / 1000 << "."
        << (driver % 1000) / 10 << "\n";

    double memoryGB =
        static_cast<double>(
            gCudaManager->getDeviceTotalMemBytes()
            ) / (1024.0 * 1024.0 * 1024.0);

    std::cout << "Pamiec GPU: "
        << memoryGB << " GiB\n";

    std::cout << "Multiprocesory SM: "
        << gCudaManager->getMultiprocessorCount()
        << "\n";

    std::cout << "================================\n";
}

// Inicjalizacja PhysX GPU
bool initPhysX()
{
    gFoundation = PxCreateFoundation(
        PX_PHYSICS_VERSION,
        gAllocator,
        gErrorCallback
    );

    if (!gFoundation)
        return false;

    gPhysics = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *gFoundation,
        PxTolerancesScale()
    );

    if (!gPhysics)
        return false;

    // Tworzenie kontekstu NVIDIA CUDA
    PxCudaContextManagerDesc cudaDesc;

    gCudaManager = PxCreateCudaContextManager(
        *gFoundation,
        cudaDesc
    );

    if (!gCudaManager || !gCudaManager->contextIsValid())
    {
        std::cerr << "BLAD: Kontekst CUDA jest niedostepny!\n";
        return false;
    }

    std::cout << "Kontekst CUDA jest poprawny.\n";
    printGPUInfo();

    // Konfiguracja sceny
    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());

    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);

    gDispatcher = PxDefaultCpuDispatcherCreate(2);

    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    // Wlaczenie GPU
    sceneDesc.cudaContextManager = gCudaManager;
    sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
    sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
    sceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;

    gScene = gPhysics->createScene(sceneDesc);

    if (!gScene)
    {
        std::cerr << "Nie udalo sie stworzyc sceny!\n";
        return false;
    }


    std::cout << "\n========== PHYSX GPU ==========\n";

    bool gpuDynamics =
        gScene->getFlags().isSet(
            PxSceneFlag::eENABLE_GPU_DYNAMICS
        );

    bool gpuBroadPhase =
        gScene->getBroadPhaseType() ==
        PxBroadPhaseType::eGPU;

    std::cout << "GPU Dynamics: "
        << (gpuDynamics ? "WLACZONE" : "WYLACZONE")
        << "\n";

    std::cout << "GPU Broad Phase: "
        << (gpuBroadPhase ? "WLACZONE" : "WYLACZONE")
        << "\n";

    if (gpuDynamics && gpuBroadPhase)
    {
        std::cout << "PhysX: SCENA SKONFIGUROWANA NA GPU\n";
    }
    else
    {
        std::cout << "UWAGA: Nie wszystkie funkcje GPU sa wlaczone!\n";
    }

    std::cout << "================================\n";


    gMaterial = gPhysics->createMaterial(
        0.6f,  // tarcie statyczne
        0.5f,  // tarcie dynamiczne
        0.2f   // wspolczynnik odbicia
    );

    if (!gMaterial)
        return false;

    // Nieruchoma podloga
    PxRigidStatic* ground = PxCreatePlane(
        *gPhysics,
        PxPlane(0, 1, 0, 0),
        *gMaterial
    );

    if (!ground)
        return false;

    gScene->addActor(*ground);

    // 500 szescianow (10 x 10 x 5)
    for (int y = 0; y < 10; ++y)
    {
        for (int x = 0; x < 10; ++x)
        {
            for (int z = 0; z < 5; ++z)
            {
                PxVec3 position(
                    (x - 4.5f) * 2.1f,
                    3.0f + y * 2.1f,
                    (z - 2.0f) * 2.1f
                );

                PxRigidDynamic* body = PxCreateDynamic(
                    *gPhysics,
                    PxTransform(position),
                    PxBoxGeometry(1.0f, 1.0f, 1.0f),
                    *gMaterial,
                    1.0f
                );

                if (!body)
                    return false;

                gScene->addActor(*body);
                gCubes.push_back(body);
            }
        }
    }

    std::cout << "Utworzono "
        << gCubes.size()
        << " szescianow.\n";

    std::cout << "Scena skonfigurowana do PhysX GPU.\n";

    return true;
}

// Aktualizacja symulacji
void updatePhysics()
{
    if (!gSimulationRunning || !gScene)
        return;

    gScene->simulate(1.0f / 60.0f);
    gScene->fetchResults(true);
}

// Renderowanie
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(
        34.0, 28.0, 42.0,
        0.0, 8.0, 0.0,
        0.0, 1.0, 0.0
    );

    // Zielona podloga
    glColor3f(0.25f, 0.55f, 0.30f);

    glBegin(GL_QUADS);
    glVertex3f(-100.0f, 0.0f, -100.0f);
    glVertex3f(-100.0f, 0.0f, 100.0f);
    glVertex3f(100.0f, 0.0f, 100.0f);
    glVertex3f(100.0f, 0.0f, -100.0f);
    glEnd();

    // Renderowanie bryl z PhysX
    for (size_t i = 0; i < gCubes.size(); ++i)
    {
        PxTransform pose = gCubes[i]->getGlobalPose();
        PxMat44 matrix(pose);

        glPushMatrix();

        glMultMatrixf(&matrix.column0.x);

        // Kolor zalezy od indeksu
        if (i % 3 == 0)
            glColor3f(0.95f, 0.25f, 0.20f);
        else if (i % 3 == 1)
            glColor3f(0.20f, 0.50f, 0.95f);
        else
            glColor3f(0.95f, 0.75f, 0.15f);

        glutSolidCube(2.0);

        glPopMatrix();
    }

    glutSwapBuffers();
}

void timer(int value)
{
    updatePhysics();
    glutPostRedisplay();

    glutTimerFunc(16, timer, 0);
}

void reshape(int width, int height)
{
    if (height == 0)
        height = 1;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        45.0,
        static_cast<double>(width) / height,
        0.1,
        300.0
    );

    glMatrixMode(GL_MODELVIEW);
}

// Sterowanie klawiatura
void keyboard(unsigned char key, int x, int y)
{
    if (key == 'p' || key == 'P')
    {
        gSimulationRunning = !gSimulationRunning;
    }

    if (key == 27)
    {
        glutLeaveMainLoop();
    }
}

// Zwalnianie zasobow
void cleanup()
{
    gCubes.clear();

    if (gScene)
    {
        gScene->release();
        gScene = nullptr;
    }

    if (gMaterial)
    {
        gMaterial->release();
        gMaterial = nullptr;
    }

    if (gDispatcher)
    {
        gDispatcher->release();
        gDispatcher = nullptr;
    }

    if (gCudaManager)
    {
        gCudaManager->release();
        gCudaManager = nullptr;
    }

    if (gPhysics)
    {
        gPhysics->release();
        gPhysics = nullptr;
    }

    if (gFoundation)
    {
        gFoundation->release();
        gFoundation = nullptr;
    }
}





int main(int argc, char** argv)
{
    if (!initPhysX())
    {
        std::cerr << "Blad inicjalizacji PhysX GPU.\n";
        cleanup();
        return 1;
    }

    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH
    );

    glutInitWindowSize(1100, 750);

    glutSetOption(
        GLUT_ACTION_ON_WINDOW_CLOSE,
        GLUT_ACTION_GLUTMAINLOOP_RETURNS
    );

    glutCreateWindow("PhysX GPU + FreeGLUT - RTX 5050");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.10f, 0.12f, 0.16f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);

    std::cout << "P - pauza / wznowienie\n";
    std::cout << "ESC - zamknij program\n";

    glutMainLoop();

    cleanup();

    return 0;
}
