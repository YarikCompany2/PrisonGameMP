#pragma once
#include "raylib.h"
#include "Entity.hpp"
#include "Tilemap.hpp"
#include "WorldObjects.hpp"
#include "Network.hpp"
#include <cstdint>

enum InputFlags {
    INPUT_NONE     = 0,
    INPUT_UP       = 1 << 0,
    INPUT_DOWN     = 1 << 1,
    INPUT_LEFT     = 1 << 2,
    INPUT_RIGHT    = 1 << 3,
    INPUT_INTERACT = 1 << 4,
    INPUT_ATTACK   = 1 << 5
};

enum GameState {
    STATE_MENU = 0,
    STATE_ENTER_IP,
    STATE_PLAYING
};

class Game {
public:
    static const int MAX_ENTITIES = 32;

    Game(int windowWidth, int windowHeight, const char* windowTitle);
    ~Game();

    void Run();

private:
    void UpdateMenu();
    void UpdateEnterIP();
    void UpdatePlaying(float deltaTime);

    void DrawMenu();
    void DrawEnterIP();
    void DrawPlaying();
    void DrawEntity(const Entity& currentEntity) const;

    void ToggleStretchFullscreen();

    uint8_t ReadLocalInput() const;
    void SimulateEntityMovement(Entity& entity, uint8_t inputButtons, float deltaTime);

    GameState currentState;
    NetworkManager network;

    char ipInputBuffer[32];
    int ipInputLength;

    Tilemap prisonMap;
    WorldObjects worldObjects;
    Entity entities[MAX_ENTITIES];
    int entityCount;

    bool showDebugObjects;

    int activeLayer;

    int viewWidth;
    int viewHeight;
    RenderTexture2D virtualScreen;
    Camera2D worldCamera;
};