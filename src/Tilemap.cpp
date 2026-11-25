#include "Tilemap.hpp"
#include "WorldObjects.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

enum TileFlagType {
    TILE_FLAG_SOLID = 0,
    TILE_FLAG_CUTTABLE = 1,
    TILE_FLAG_CHIPPABLE = 2
};

Tilemap::Tilemap() {
    mapWidth = 0;
    mapHeight = 0;
    prisonName[0] = '\0';
    tilesetName[0] = '\0';
    memset(tiles, 0, sizeof(tiles));
    memset(solidGrid, 0, sizeof(solidGrid));

    isTextureLoaded = false;
    tilesetTexture = (Texture2D){ 0 };

    ResetTileDatabase();
}

Tilemap::~Tilemap() {
    Unload();
}

void Tilemap::ResetTileDatabase() {
    memset(tileDatabase, 0, sizeof(tileDatabase));
}

bool Tilemap::LoadTileset(const char* texturePath) {
    if (isTextureLoaded) {
        Unload();
    }

    Image img = LoadImage(texturePath);
    if (img.data != nullptr) {
        ImageColorReplace(&img, WHITE, BLANK);
        tilesetTexture = LoadTextureFromImage(img);
        UnloadImage(img);
    } else {
        tilesetTexture = (Texture2D){ 0 };
    }

    isTextureLoaded = (tilesetTexture.id > 0);

    if (isTextureLoaded) {
        std::cout << "Текстуру тайлсету завантажено: " << texturePath
                  << " [" << tilesetTexture.width << "x" << tilesetTexture.height << "]" << std::endl;
        return true;
    } else {
        std::cerr << "Помилка: не вдалося завантажити текстуру: " << texturePath << std::endl;
        return false;
    }
}

void Tilemap::Unload() {
    if (isTextureLoaded) {
        UnloadTexture(tilesetTexture);
        isTextureLoaded = false;
        tilesetTexture = (Texture2D){ 0 };
    }
}

void Tilemap::ApplyRangeListToDatabase(const char* listStr, int flagType) {
    const char* ptr = listStr;

    while (*ptr != '\0') {
        while (*ptr == ' ' || *ptr == '\t' || *ptr == ',') {
            ptr++;
        }
        if (*ptr == '\0' || *ptr == '#') break;

        char* endPtr = nullptr;
        long startId = std::strtol(ptr, &endPtr, 10);
        if (endPtr == ptr) {
            ptr++;
            continue;
        }
        ptr = endPtr;

        long endId = startId;
        while (*ptr == ' ' || *ptr == '\t') ptr++;

        if (*ptr == '-') {
            ptr++;
            long parsedEnd = std::strtol(ptr, &endPtr, 10);
            if (endPtr != ptr) {
                endId = parsedEnd;
                ptr = endPtr;
            }
        }

        if (startId > endId) {
            long tmp = startId;
            startId = endId;
            endId = tmp;
        }

        for (long id = startId; id <= endId; ++id) {
            if (id >= 0 && id < 256) {
                if (flagType == TILE_FLAG_SOLID)     tileDatabase[id].isSolid = true;
                if (flagType == TILE_FLAG_CUTTABLE)  tileDatabase[id].isCuttable = true;
                if (flagType == TILE_FLAG_CHIPPABLE) tileDatabase[id].isChippable = true;
            }
        }
    }
}

bool Tilemap::LoadTilesetConfig(const char* configPath) {
    std::ifstream fileStream(configPath);
    if (!fileStream.is_open()) {
        std::cerr << "Попередження: конфіг колізій не знайдено: " << configPath << std::endl;
        return false;
    }

    ResetTileDatabase();

    std::string line;
    std::string section = "";

    while (std::getline(fileStream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }

        if (section == "Collision") {
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;

            std::string key = line.substr(0, eqPos);
            std::string val = line.substr(eqPos + 1);

            if (key == "Solid") {
                ApplyRangeListToDatabase(val.c_str(), TILE_FLAG_SOLID);
            } else if (key == "Cuttable") {
                ApplyRangeListToDatabase(val.c_str(), TILE_FLAG_CUTTABLE);
            } else if (key == "Chippable") {
                ApplyRangeListToDatabase(val.c_str(), TILE_FLAG_CHIPPABLE);
            }
        }
    }

    fileStream.close();
    RebuildSolidGrid();
    std::cout << "Конфіг тайлсету завантажено: " << configPath << std::endl;
    return true;
}

void Tilemap::RebuildSolidGrid() {
    for (int layer = 0; layer < LAYER_COUNT; ++layer) {
        for (int row = 0; row < MAX_MAP_HEIGHT; ++row) {
            for (int col = 0; col < MAX_MAP_WIDTH; ++col) {
                int tileId = tiles[layer][row][col];
                if (tileId == 0) {
                    solidGrid[layer][row][col] = (layer != LAYER_GROUND);
                } else if (tileId > 0 && tileId < 256) {
                    solidGrid[layer][row][col] = tileDatabase[tileId].isSolid;
                } else {
                    solidGrid[layer][row][col] = true;
                }
            }
        }
    }
}

bool Tilemap::LoadFromCmap(const char* filePath, WorldObjects* outObjects) {
    std::ifstream fileStream(filePath);
    if (!fileStream.is_open()) {
        std::cerr << "Помилка: не вдалося відкрити файл карти: " << filePath << std::endl;
        return false;
    }

    std::string currentLine;
    std::string currentSection = "";
    mapWidth = 0;
    mapHeight = 0;
    memset(tiles, 0, sizeof(tiles));

    if (outObjects) {
        outObjects->Clear();
    }

    while (std::getline(fileStream, currentLine)) {
        if (!currentLine.empty() && currentLine.back() == '\r') currentLine.pop_back();
        if (currentLine.empty()) continue;

        if (currentLine.front() == '[' && currentLine.back() == ']') {
            currentSection = currentLine.substr(1, currentLine.size() - 2);
            continue;
        }

        if (currentSection == "Info") {
            size_t separatorPos = currentLine.find('=');
            if (separatorPos != std::string::npos) {
                std::string key = currentLine.substr(0, separatorPos);
                std::string value = currentLine.substr(separatorPos + 1);

                if (key == "MapName" || key == "Name") {
                    strncpy(prisonName, value.c_str(), sizeof(prisonName) - 1);
                    prisonName[sizeof(prisonName) - 1] = '\0';
                } else if (key == "Tileset") {
                    strncpy(tilesetName, value.c_str(), sizeof(tilesetName) - 1);
                    tilesetName[sizeof(tilesetName) - 1] = '\0';

                    char pathBuffer[128];
                    std::snprintf(pathBuffer, sizeof(pathBuffer), "assets/textures/tiles_%s.png", tilesetName);
                    LoadTileset(pathBuffer);

                    std::snprintf(pathBuffer, sizeof(pathBuffer), "assets/tilesets/%s.cfg", tilesetName);
                    LoadTilesetConfig(pathBuffer);
                }
            }
        }
        else if (currentSection == "Tiles" ||
                 currentSection == "Vents" ||
                 currentSection == "Roof"  ||
                 currentSection == "Underground") {
            
            int targetLayer = LAYER_GROUND;
            if (currentSection == "Vents") {
                targetLayer = LAYER_VENTS;
            } else if (currentSection == "Roof") {
                targetLayer = LAYER_ROOF;
            } else if (currentSection == "Underground") {
                targetLayer = LAYER_UNDERGROUND;
            }

            size_t separatorPos = currentLine.find('=');
            if (separatorPos != std::string::npos) {
                int rowIndex = std::stoi(currentLine.substr(0, separatorPos));
                std::string tileDataString = currentLine.substr(separatorPos + 1);

                if (rowIndex < 0 || rowIndex >= MAX_MAP_HEIGHT) continue;

                std::stringstream stringStream(tileDataString);
                std::string tileToken;
                int columnIndex = 0;

                while (std::getline(stringStream, tileToken, '_')) {
                    if (tileToken.empty()) continue;

                    if (columnIndex < MAX_MAP_WIDTH) {
                        tiles[targetLayer][rowIndex][columnIndex] = std::stoi(tileToken);
                        columnIndex++;
                    }
                }

                if (columnIndex > mapWidth) mapWidth = columnIndex;
                if (rowIndex + 1 > mapHeight) mapHeight = rowIndex + 1;
            }
        }
        else if (currentSection == "Objects") {
            if (!outObjects) continue;

            size_t separatorPos = currentLine.find('=');
            if (separatorPos == std::string::npos) continue;

            int tileX = 0;
            int tileY = 0;
            int typeId = 0;
            int layer = 0;
            if (std::sscanf(currentLine.c_str() + separatorPos + 1, "%dx%dx%dx%d",
                            &tileX, &tileY, &typeId, &layer) == 4) {
                outObjects->AddObject(tileX, tileY, typeId, layer);
            }
        }
    }

    fileStream.close();
    RebuildSolidGrid();

    if (outObjects) {
        outObjects->LoadAtlas("assets/textures/objects.png", "assets/tilesets/objects.cfg");
        outObjects->ResolveOrientations(*this);
    }

    std::cout << "Успішно завантажено оригінальну карту: \"" << prisonName << "\" ["
              << mapWidth << "x" << mapHeight << " тайлів, "
              << (outObjects ? outObjects->GetCount() : 0) << " об'єктів]" << std::endl;
    return true;
}

void Tilemap::Draw(int activeLayer, float alpha) const {
    if (activeLayer < 0 || activeLayer >= LAYER_COUNT) activeLayer = LAYER_GROUND;
    const int TILES_PER_COLUMN = 25;
    const int VOID_TILE_ID = 100;

    Color tint = Fade(WHITE, alpha);

    for (int row = 0; row < mapHeight; ++row) {
        for (int column = 0; column < mapWidth; ++column) {
            int tileId = tiles[activeLayer][row][column];

            if (activeLayer == LAYER_VENTS && tileId == VOID_TILE_ID) {
                continue;
            }

            Rectangle destinationRectangle = {
                (float)(column * SCREEN_TILE_SIZE),
                (float)(row * SCREEN_TILE_SIZE),
                (float)SCREEN_TILE_SIZE,
                (float)SCREEN_TILE_SIZE
            };

            if (tileId == 0) {
                if (activeLayer == LAYER_GROUND && isTextureLoaded) {
                    Rectangle grassSource = {
                        (float)(1 * SOURCE_TILE_SIZE),
                        (float)(24 * SOURCE_TILE_SIZE),
                        (float)SOURCE_TILE_SIZE,
                        (float)SOURCE_TILE_SIZE
                    };
                    DrawTexturePro(tilesetTexture, grassSource, destinationRectangle, (Vector2){ 0, 0 }, 0.0f, tint);
                }
                continue;
            }

            int atlasColumn = (tileId - 1) / TILES_PER_COLUMN;
            int atlasRow    = (tileId - 1) % TILES_PER_COLUMN;
            if (atlasColumn >= 4) atlasColumn = 3;

            Rectangle sourceRectangle = {
                (float)(atlasColumn * SOURCE_TILE_SIZE),
                (float)(atlasRow * SOURCE_TILE_SIZE),
                (float)SOURCE_TILE_SIZE,
                (float)SOURCE_TILE_SIZE
            };

            if (isTextureLoaded) {
                DrawTexturePro(tilesetTexture, sourceRectangle, destinationRectangle, (Vector2){ 0, 0 }, 0.0f, tint);
            }
        }
    }
}

void Tilemap::DrawSolidDebug(int activeLayer) const {
    if (activeLayer < 0 || activeLayer >= LAYER_COUNT) activeLayer = LAYER_GROUND;

    for (int row = 0; row < mapHeight; ++row) {
        for (int col = 0; col < mapWidth; ++col) {
            if (solidGrid[activeLayer][row][col]) {
                DrawRectangleLines(
                    col * SCREEN_TILE_SIZE,
                    row * SCREEN_TILE_SIZE,
                    SCREEN_TILE_SIZE,
                    SCREEN_TILE_SIZE,
                    Fade(RED, 0.7f)
                );
            }
        }
    }
}

bool Tilemap::IsSolidAt(int gridColumn, int gridRow, int layer) const {
    if (layer < 0 || layer >= LAYER_COUNT) layer = LAYER_GROUND;
    if (gridColumn < 0 || gridColumn >= mapWidth || gridRow < 0 || gridRow >= mapHeight) {
        return true;
    }
    return solidGrid[layer][gridRow][gridColumn];
}

void Tilemap::SetSolidAt(int gridColumn, int gridRow, bool solid, int layer) {
    if (layer < 0 || layer >= LAYER_COUNT) layer = LAYER_GROUND;
    if (gridColumn < 0 || gridColumn >= mapWidth || gridRow < 0 || gridRow >= mapHeight) {
        return;
    }
    solidGrid[layer][gridRow][gridColumn] = solid;
}

bool Tilemap::CheckCollisionAABB(Rectangle box, int layer) const {
    int leftTile   = (int)(box.x / SCREEN_TILE_SIZE);
    int rightTile  = (int)((box.x + box.width - 0.01f) / SCREEN_TILE_SIZE);
    int topTile    = (int)(box.y / SCREEN_TILE_SIZE);
    int bottomTile = (int)((box.y + box.height - 0.01f) / SCREEN_TILE_SIZE);

    for (int row = topTile; row <= bottomTile; ++row) {
        for (int col = leftTile; col <= rightTile; ++col) {
            if (IsSolidAt(col, row, layer)) {
                return true;
            }
        }
    }
    return false;
}