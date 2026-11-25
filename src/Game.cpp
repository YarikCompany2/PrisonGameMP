#include "Game.hpp"
#include <cmath>
#include <cstring>

Game::Game(int windowWidth, int windowHeight, const char* windowTitle) {
    InitWindow(windowWidth, windowHeight, windowTitle);
    SetTargetFPS(60);

    prisonMap.LoadFromCmap("assets/maps/central_park.cmap", &worldObjects);

    entityCount = 0;
    memset(entities, 0, sizeof(entities));

    Color playerColors[4] = { BLUE, RED, GREEN, YELLOW };
    const char* playerNames[4] = { "Player 1", "Player 2", "Player 3", "Player 4" };

    for (int i = 0; i < NetworkManager::MAX_NET_PLAYERS; ++i) {
        ControllerType ctrl = (i == 0) ? CONTROLLER_LOCAL_PLAYER : CONTROLLER_NETWORK_PLAYER;
        entities[entityCount] = EntityFactory::CreatePlayer(
            i, playerNames[i], NetworkManager::GetPlayerSpawnPosition(i), playerColors[i], ctrl
        );
        entities[entityCount].isActive = false;
        entityCount++;
    }

    entities[entityCount++] = EntityFactory::CreateOfficer(
        4, (Vector2){ 2016.0f, 1312.0f }
    );

    currentState = STATE_MENU;
    strcpy(ipInputBuffer, "127.0.0.1");
    ipInputLength = (int)strlen(ipInputBuffer);
    showDebugObjects = false;

    viewWidth = windowWidth;
    viewHeight = windowHeight;
    virtualScreen = LoadRenderTexture(viewWidth, viewHeight);
    SetTextureFilter(virtualScreen.texture, TEXTURE_FILTER_POINT);

    activeLayer = Tilemap::LAYER_GROUND;

    worldCamera.target = entities[0].transform.position;
    worldCamera.offset = (Vector2){ (float)viewWidth / 2.0f, (float)viewHeight / 2.0f };
    worldCamera.rotation = 0.0f;
    worldCamera.zoom = 1.5f;
}

Game::~Game() {
    network.Shutdown();
    worldObjects.Unload();
    prisonMap.Unload();
    UnloadRenderTexture(virtualScreen);
    CloseWindow();
}

void Game::ToggleStretchFullscreen() {
    if (!IsWindowFullscreen()) {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    } else {
        ToggleFullscreen();
        SetWindowSize(viewWidth, viewHeight);
    }
}

uint8_t Game::ReadLocalInput() const {
    uint8_t buttons = INPUT_NONE;

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    buttons |= INPUT_UP;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  buttons |= INPUT_DOWN;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  buttons |= INPUT_LEFT;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) buttons |= INPUT_RIGHT;
    if (IsKeyPressed(KEY_E))                      buttons |= INPUT_INTERACT;
    if (IsKeyPressed(KEY_SPACE))                  buttons |= INPUT_ATTACK;

    return buttons;
}

void Game::SimulateEntityMovement(Entity& entity, uint8_t inputButtons, float deltaTime) {
    float moveX = 0.0f;
    float moveY = 0.0f;

    if (inputButtons & INPUT_UP)    moveY -= 1.0f;
    if (inputButtons & INPUT_DOWN)  moveY += 1.0f;
    if (inputButtons & INPUT_LEFT)  moveX -= 1.0f;
    if (inputButtons & INPUT_RIGHT) moveX += 1.0f;

    if (moveX != 0.0f && moveY != 0.0f) {
        const float invSqrt2 = 0.70710678f;
        moveX *= invSqrt2;
        moveY *= invSqrt2;
    }

    if (moveX == 0.0f && moveY == 0.0f) return;

    float movementSpeed = 160.0f;
    if (entity.hasAttributes) {
        movementSpeed += entity.attributes.speed * 2.5f;
    }

    bool ignoreCollisions = showDebugObjects &&
        (entity.controller == CONTROLLER_LOCAL_PLAYER || entity.controller == CONTROLLER_NETWORK_PLAYER);

    float nextX = entity.transform.position.x + moveX * movementSpeed * deltaTime;
    Rectangle hitboxX = { nextX, entity.transform.position.y, entity.transform.size.x, entity.transform.size.y };
    if (entity.isNoclip ||
        (!prisonMap.CheckCollisionAABB(hitboxX, entity.currentLayer) &&
         !worldObjects.CheckCollisionAABB(hitboxX, entity.currentLayer))) {
        entity.transform.position.x = nextX;
    }

    float nextY = entity.transform.position.y + moveY * movementSpeed * deltaTime;
    Rectangle hitboxY = { entity.transform.position.x, nextY, entity.transform.size.x, entity.transform.size.y };
    if (entity.isNoclip ||
        (!prisonMap.CheckCollisionAABB(hitboxY, entity.currentLayer) &&
         !worldObjects.CheckCollisionAABB(hitboxY, entity.currentLayer))) {
        entity.transform.position.y = nextY;
    }
}

void Game::UpdateMenu() {
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
        if (network.StartHost(entities)) currentState = STATE_PLAYING;
    }
    else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
        if (network.ConnectToHost("127.0.0.1")) currentState = STATE_PLAYING;
    }
    else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
        ipInputBuffer[0] = '\0';
        ipInputLength = 0;
        currentState = STATE_ENTER_IP;
    }
}

void Game::UpdateEnterIP() {
    int key = GetCharPressed();
    while (key > 0) {
        if (((key >= '0' && key <= '9') || key == '.') && ipInputLength < 30) {
            ipInputBuffer[ipInputLength++] = (char)key;
            ipInputBuffer[ipInputLength] = '\0';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && ipInputLength > 0) {
        ipInputBuffer[--ipInputLength] = '\0';
    }

    if (IsKeyPressed(KEY_ENTER) && ipInputLength > 0) {
        if (network.ConnectToHost(ipInputBuffer)) currentState = STATE_PLAYING;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        currentState = STATE_MENU;
    }
}

void Game::UpdatePlaying(float deltaTime) {
    int myIdx = network.GetLocalPlayerIndex();

    if (IsKeyPressed(KEY_F1)) {
        showDebugObjects = !showDebugObjects;
        if (myIdx >= 0 && myIdx < entityCount) {
            entities[myIdx].isNoclip = showDebugObjects;
        }
    }

    if (IsKeyPressed(KEY_F2)) {
        activeLayer = (activeLayer + 1) % Tilemap::LAYER_COUNT;
        if (myIdx >= 0 && myIdx < entityCount) {
            entities[myIdx].currentLayer = activeLayer;
        }
    }

    worldObjects.Update(deltaTime);

    uint8_t localButtons = ReadLocalInput();

    if (network.GetMode() == NET_MODE_HOST) {
        if (entities[0].isActive) {
            SimulateEntityMovement(entities[0], localButtons, deltaTime);
        }
        for (int i = 1; i < NetworkManager::MAX_NET_PLAYERS; ++i) {
            if (entities[i].isActive) {
                SimulateEntityMovement(entities[i], network.GetPlayerInput(i), deltaTime);
            }
        }
    }

    network.Sync(entities, entityCount, localButtons, deltaTime);

    myIdx = network.GetLocalPlayerIndex();
    if (myIdx >= 0 && myIdx < entityCount) {
        entities[myIdx].currentLayer = activeLayer;
        entities[myIdx].isNoclip = showDebugObjects;
        worldCamera.target = entities[myIdx].transform.position;
    }
}

void Game::DrawMenu() {
    ClearBackground((Color){ 24, 28, 36, 255 });

    DrawText("PRISON GAME - MULTIPLAYER", 80, 80, 36, GOLD);
    DrawText(TextFormat("Loaded Map: %s", prisonMap.GetPrisonName()), 80, 130, 20, LIGHTGRAY);

    DrawText("[1] Host Game (Listen Server - Player 1)", 80, 210, 22, SKYBLUE);
    DrawText("[2] Join Localhost 127.0.0.1 (Player 2)", 80, 250, 22, GREEN);
    DrawText("[3] Join by IP Address", 80, 290, 22, ORANGE);
}

void Game::DrawEnterIP() {
    ClearBackground((Color){ 24, 28, 36, 255 });

    DrawText("Enter Host IP Address:", 80, 120, 26, GOLD);
    DrawRectangle(80, 170, 360, 44, DARKGRAY);
    DrawRectangleLines(80, 170, 360, 44, WHITE);
    DrawText(ipInputBuffer, 95, 180, 24, RAYWHITE);

    DrawText("Press ENTER to connect | ESC to go back", 80, 235, 18, LIGHTGRAY);
}

void Game::DrawPlaying() {
    ClearBackground(BLACK);

    BeginMode2D(worldCamera);

        if (activeLayer == Tilemap::LAYER_VENTS) {
            prisonMap.Draw(Tilemap::LAYER_GROUND, 1.0f);
            worldObjects.Draw(Tilemap::LAYER_GROUND);

            for (int i = 0; i < entityCount; ++i) {
                if (entities[i].isActive && entities[i].currentLayer == Tilemap::LAYER_GROUND) {
                    DrawEntity(entities[i]);
                }
            }

            DrawRectangle(
                0, 0,
                prisonMap.GetWidth() * Tilemap::SCREEN_TILE_SIZE,
                prisonMap.GetHeight() * Tilemap::SCREEN_TILE_SIZE,
                Fade(BLACK, 0.55f)
            );

            prisonMap.Draw(Tilemap::LAYER_VENTS, 0.85f);
            worldObjects.Draw(Tilemap::LAYER_VENTS);

            for (int i = 0; i < entityCount; ++i) {
                if (entities[i].isActive && entities[i].currentLayer == Tilemap::LAYER_VENTS) {
                    DrawEntity(entities[i]);
                }
            }
        } else {
            prisonMap.Draw(activeLayer, 1.0f);
            worldObjects.Draw(activeLayer);

            for (int i = 0; i < entityCount; ++i) {
                if (entities[i].isActive && entities[i].currentLayer == activeLayer) {
                    DrawEntity(entities[i]);
                }
            }
        }

        if (showDebugObjects) {
            prisonMap.DrawSolidDebug(activeLayer);
            worldObjects.DrawDebug(activeLayer);
        }

    EndMode2D();

    int myIdx = network.GetLocalPlayerIndex();
    const char* roleText = (network.GetMode() == NET_MODE_HOST)
        ? TextFormat("HOST (Player 1) | Players: %d/4", network.GetConnectedCount())
        : (myIdx >= 0 ? TextFormat("CLIENT (Player %d)", myIdx + 1) : "CLIENT (Connecting...)");

    DrawText(TextFormat("Prison: %s | %s", prisonMap.GetPrisonName(), roleText), 20, 20, 18, GOLD);

    const char* layerNames[Tilemap::LAYER_COUNT] = { "UNDERGROUND", "GROUND", "VENTS", "ROOF" };
    DrawText(TextFormat("Layer [F2]: %s", layerNames[activeLayer]), 20, 46, 16, SKYBLUE);
}

void Game::DrawEntity(const Entity& currentEntity) const {
    const Vector2& position = currentEntity.transform.position;
    const Vector2& size = currentEntity.transform.size;

    DrawRectangleV(position, size, currentEntity.color);

    if (currentEntity.hasHealth) {
        float healthPercent = (float)currentEntity.health.currentHealth / (float)currentEntity.health.maxHealth;
        DrawRectangle((int)position.x, (int)position.y - 12, (int)size.x, 4, RED);
        DrawRectangle((int)position.x, (int)position.y - 12, (int)(size.x * healthPercent), 4, GREEN);
    }

    if (currentEntity.hasPlayerNeeds) {
        float fatiguePercent = (float)currentEntity.playerNeeds.fatigue / 100.0f;
        DrawRectangle((int)position.x, (int)position.y - 6, (int)size.x, 3, DARKGRAY);
        DrawRectangle((int)position.x, (int)position.y - 6, (int)(size.x * fatiguePercent), 3, SKYBLUE);
    }

    if (currentEntity.hasSocialStats) {
        DrawText(TextFormat("%s [OP:%d]", currentEntity.name, currentEntity.social.opinion),
                 (int)position.x - 10, (int)position.y - 25, 10, RAYWHITE);
    } else {
        DrawText(currentEntity.name, (int)position.x - 5, (int)position.y - 25, 10, RAYWHITE);
    }
}

void Game::Run() {
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        bool altEnter = IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT));
        if (IsKeyPressed(KEY_F11) || altEnter) {
            ToggleStretchFullscreen();
        }

        if (currentState == STATE_MENU) {
            UpdateMenu();
        } else if (currentState == STATE_ENTER_IP) {
            UpdateEnterIP();
        } else {
            UpdatePlaying(deltaTime);
        }

        BeginTextureMode(virtualScreen);
            if (currentState == STATE_MENU) {
                DrawMenu();
            } else if (currentState == STATE_ENTER_IP) {
                DrawEnterIP();
            } else {
                DrawPlaying();
            }
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            Rectangle source = {
                0.0f,
                0.0f,
                (float)virtualScreen.texture.width,
                -(float)virtualScreen.texture.height
            };
            Rectangle destination = {
                0.0f,
                0.0f,
                (float)GetScreenWidth(),
                (float)GetScreenHeight()
            };
            DrawTexturePro(virtualScreen.texture, source, destination, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        EndDrawing();
    }
}