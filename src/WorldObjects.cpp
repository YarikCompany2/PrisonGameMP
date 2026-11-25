#include "WorldObjects.hpp"
#include "Tilemap.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

WorldObjects::WorldObjects() {
    objectCount = 0;
    isTextureLoaded = false;
    objectsTexture = (Texture2D){ 0 };
    memset(spriteSets, 0, sizeof(spriteSets));
    memset(anchorBottom, 0, sizeof(anchorBottom));
    memset(defaultSolid, 0, sizeof(defaultSolid));
    memset(doorTypes, 0, sizeof(doorTypes));
    memset(climbableTypes, 0, sizeof(climbableTypes));
    memset(unscrewableTypes, 0, sizeof(unscrewableTypes));
    memset(objects, 0, sizeof(objects));
}

WorldObjects::~WorldObjects() {
    Unload();
}

void WorldObjects::Clear() {
    objectCount = 0;
    memset(objects, 0, sizeof(objects));
}

void WorldObjects::Unload() {
    if (isTextureLoaded) {
        UnloadTexture(objectsTexture);
        isTextureLoaded = false;
        objectsTexture = (Texture2D){ 0 };
    }
}

void WorldObjects::ParseRangeList(const char* listStr, bool targetArray[256]) {
    const char* ptr = listStr;
    while (*ptr != '\0') {
        while (*ptr == ' ' || *ptr == '\t' || *ptr == ',') ptr++;
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
                targetArray[id] = true;
            }
        }
    }
}

bool WorldObjects::IsDoorType(int typeId) const {
    return (typeId >= 0 && typeId < 256) ? doorTypes[typeId] : false;
}

bool WorldObjects::IsDefaultSolidType(int typeId) const {
    if (typeId < 0 || typeId >= 256) return false;
    return doorTypes[typeId] || defaultSolid[typeId] || unscrewableTypes[typeId] || cuttableTypes[typeId];
}

bool WorldObjects::IsClimbableType(int typeId) const {
    return (typeId >= 0 && typeId < 256) ? climbableTypes[typeId] : false;
}

bool WorldObjects::IsUnscrewableType(int typeId) const {
    return (typeId >= 0 && typeId < 256) ? unscrewableTypes[typeId] : false;
}

bool WorldObjects::IsCuttableType(int typeId) const {
    return (typeId >= 0 && typeId < 256) ? cuttableTypes[typeId] : false;
}

bool WorldObjects::LoadAtlas(const char* texturePath, const char* configPath) {
    Unload();
    memset(spriteSets, 0, sizeof(spriteSets));
    memset(anchorBottom, 0, sizeof(anchorBottom));
    memset(defaultSolid, 0, sizeof(defaultSolid));
    memset(doorTypes, 0, sizeof(doorTypes));
    memset(climbableTypes, 0, sizeof(climbableTypes));
    memset(unscrewableTypes, 0, sizeof(unscrewableTypes));
    memset(cuttableTypes, 0, sizeof(cuttableTypes));

    objectsTexture = LoadTexture(texturePath);
    isTextureLoaded = (objectsTexture.id > 0);
    if (!isTextureLoaded) {
        std::cerr << "Попередження: атлас об'єктів не знайдено: " << texturePath << std::endl;
        return false;
    }

    std::ifstream cfg(configPath);
    if (!cfg.is_open()) {
        std::cerr << "Попередження: конфіг об'єктів не знайдено: " << configPath << std::endl;
        return false;
    }

    std::string line;
    std::string section = "";
    int loadedCount = 0;

    while (std::getline(cfg, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }

        if (section == "AnchorBottom") {
            if (line.rfind("Ids=", 0) == 0) {
                ParseRangeList(line.c_str() + 4, anchorBottom);
            }
        }
        else if (section == "Collision") {
            if (line.rfind("Solid=", 0) == 0) {
                ParseRangeList(line.c_str() + 6, defaultSolid);
            } else if (line.rfind("Doors=", 0) == 0) {
                ParseRangeList(line.c_str() + 6, doorTypes);
            } else if (line.rfind("Climbable=", 0) == 0) {
                ParseRangeList(line.c_str() + 10, climbableTypes);
            } else if (line.rfind("Unscrewable=", 0) == 0) {
                ParseRangeList(line.c_str() + 12, unscrewableTypes);
            } else if (line.rfind("Cuttable=", 0) == 0) {
                ParseRangeList(line.c_str() + 9, cuttableTypes);
            }
        }
        else if (section == "Sprites") {
            int typeId = 0, dir = 0, frame = 0, x = 0, y = 0, w = 0, h = 0;
            if (std::sscanf(line.c_str(), "%d_%d_%d=%d_%d_%d_%d",
                            &typeId, &dir, &frame, &x, &y, &w, &h) == 7) {
                if (typeId >= 0 && typeId < 256 &&
                    dir >= 0 && dir < OBJ_DIR_COUNT &&
                    frame >= 0 && frame < 16) {
                    spriteSets[typeId].dirs[dir].frames[frame] = (Rectangle){
                        (float)x, (float)y, (float)w, (float)h
                    };
                    if (frame + 1 > spriteSets[typeId].dirs[dir].frameCount) {
                        spriteSets[typeId].dirs[dir].frameCount = frame + 1;
                    }
                    loadedCount++;
                }
            }
        }
    }

    for (int i = 0; i < objectCount; ++i) {
        objects[i].isSolid = IsDefaultSolidType(objects[i].typeId);
    }

    std::cout << "Атлас об'єктів завантажено: " << loadedCount << " спрайтів" << std::endl;
    return true;
}

bool WorldObjects::AddObject(int tileX, int tileY, int typeId, int layer) {
    if (objectCount >= MAX_OBJECTS) return false;

    MapObject& obj = objects[objectCount++];
    obj.tileX = tileX;
    obj.tileY = tileY;
    obj.typeId = typeId;
    obj.layer = layer;
    obj.dir = OBJ_DIR_DOWN;
    obj.currentFrame = 0;
    obj.state = 0;
    obj.integrity = 100;
    obj.animTimer = 0.0f;
    obj.isOpen = false;
    obj.isSolid = IsDefaultSolidType(typeId);
    return true;
}

void WorldObjects::ResolveOrientations(const Tilemap& map) {
    for (int i = 0; i < objectCount; ++i) {
        MapObject& obj = objects[i];
        obj.dir = OBJ_DIR_DOWN;
        obj.currentFrame = 0;

        bool wallLeft   = map.IsSolidAt(obj.tileX - 1, obj.tileY, obj.layer);
        bool wallRight  = map.IsSolidAt(obj.tileX + 1, obj.tileY, obj.layer);
        bool wallTop    = map.IsSolidAt(obj.tileX, obj.tileY - 1, obj.layer);
        bool wallBottom = map.IsSolidAt(obj.tileX, obj.tileY + 1, obj.layer);

        if (obj.typeId == 3) {
            if (wallRight && !wallLeft) {
                obj.dir = OBJ_DIR_LEFT;
            } else if (wallLeft && !wallRight) {
                obj.dir = OBJ_DIR_RIGHT;
            }
        }
        else if (obj.typeId == 51) {
            if ((wallTop && wallBottom) || ((wallTop || wallBottom) && !wallLeft && !wallRight)) {
                obj.dir = OBJ_DIR_RIGHT;
            } else {
                obj.dir = OBJ_DIR_DOWN;
            }
        }
    }
}

void WorldObjects::Update(float deltaTime) {
    for (int i = 0; i < objectCount; ++i) {
        MapObject& obj = objects[i];

        if (IsDoorType(obj.typeId) || IsUnscrewableType(obj.typeId) || IsCuttableType(obj.typeId)) {
            obj.currentFrame = obj.isOpen ? 1 : 0;
            obj.isSolid = !obj.isOpen;
        }
    }
    (void)deltaTime;
}

void WorldObjects::Draw(int activeLayer) const {
    if (!isTextureLoaded) return;
    if (activeLayer < 0 || activeLayer >= Tilemap::LAYER_COUNT) activeLayer = Tilemap::LAYER_GROUND;

    const float scale = (float)SCREEN_TILE_SIZE / (float)SOURCE_TILE_SIZE;

    for (int i = 0; i < objectCount; ++i) {
        const MapObject& obj = objects[i];
        if (obj.layer != activeLayer) continue;
        if (obj.typeId < 0 || obj.typeId >= 256) continue;

        const ObjectSpriteSet& set = spriteSets[obj.typeId];
        int dir = obj.dir;
        if (set.dirs[dir].frameCount == 0) {
            dir = OBJ_DIR_DOWN;
        }

        const ObjectAnimClip& clip = set.dirs[dir];
        if (clip.frameCount == 0) continue;

        int frameIdx = obj.currentFrame;
        if (frameIdx < 0 || frameIdx >= clip.frameCount) frameIdx = 0;

        Rectangle src = clip.frames[frameIdx];
        float dstW = src.width * scale;
        float dstH = src.height * scale;

        float drawX = (float)(obj.tileX * SCREEN_TILE_SIZE);
        float drawY = (float)(obj.tileY * SCREEN_TILE_SIZE);

        if (anchorBottom[obj.typeId]) {
            drawY = (float)((obj.tileY + 1) * SCREEN_TILE_SIZE) - dstH;
        }

        Rectangle dst = { drawX, drawY, dstW, dstH };

        DrawTexturePro(objectsTexture, src, dst, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
    }
}

void WorldObjects::DrawDebug(int activeLayer) const {
    if (activeLayer < 0 || activeLayer >= Tilemap::LAYER_COUNT) activeLayer = Tilemap::LAYER_GROUND;

    for (int i = 0; i < objectCount; ++i) {
        const MapObject& obj = objects[i];
        if (obj.layer != activeLayer) continue;

        float px = (float)(obj.tileX * SCREEN_TILE_SIZE);
        float py = (float)(obj.tileY * SCREEN_TILE_SIZE);

        Color markerColor = Fade(YELLOW, 0.35f);
        if (IsDoorType(obj.typeId)) {
            markerColor = Fade(SKYBLUE, 0.65f);
        } else if (IsClimbableType(obj.typeId)) {
            markerColor = Fade(ORANGE, 0.65f);
        } else if (IsUnscrewableType(obj.typeId) || IsCuttableType(obj.typeId)) {
            markerColor = Fade(PURPLE, 0.65f);
        } else if (obj.typeId == 50 || obj.typeId == 51 || obj.typeId == 52 || obj.typeId == 53) {
            markerColor = Fade(BROWN, 0.65f);
        } else if (obj.typeId == 61) {
            markerColor = Fade(LIME, 0.7f);
        }

        DrawRectangle((int)px + 4, (int)py + 4, SCREEN_TILE_SIZE - 8, SCREEN_TILE_SIZE - 8, markerColor);
        DrawText(TextFormat("%d", obj.typeId), (int)px + 6, (int)py + 10, 10, WHITE);
    }
}

bool WorldObjects::CheckCollisionAABB(Rectangle box, int layer) const {
    if (layer < 0 || layer >= Tilemap::LAYER_COUNT) layer = Tilemap::LAYER_GROUND;

    for (int i = 0; i < objectCount; ++i) {
        const MapObject& obj = objects[i];
        if (obj.layer != layer || !obj.isSolid) continue;

        Rectangle objRect = {
            (float)(obj.tileX * SCREEN_TILE_SIZE),
            (float)(obj.tileY * SCREEN_TILE_SIZE),
            (float)SCREEN_TILE_SIZE,
            (float)SCREEN_TILE_SIZE
        };

        if (CheckCollisionRecs(box, objRect)) {
            return true;
        }
    }
    return false;
}

MapObject* WorldObjects::GetObjectAt(int tileX, int tileY, int layer) {
    for (int i = 0; i < objectCount; ++i) {
        if (objects[i].layer == layer &&
            objects[i].tileX == tileX &&
            objects[i].tileY == tileY) {
            return &objects[i];
        }
    }
    return nullptr;
}

const MapObject* WorldObjects::GetObjectAt(int tileX, int tileY, int layer) const {
    for (int i = 0; i < objectCount; ++i) {
        if (objects[i].layer == layer &&
            objects[i].tileX == tileX &&
            objects[i].tileY == tileY) {
            return &objects[i];
        }
    }
    return nullptr;
}