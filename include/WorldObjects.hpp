#ifndef WORLD_OBJECTS_HPP
#define WORLD_OBJECTS_HPP

#include <raylib.h>

class Tilemap;

enum ObjectDirection : int {
    OBJ_DIR_DOWN  = 0,
    OBJ_DIR_RIGHT = 1,
    OBJ_DIR_UP    = 2,
    OBJ_DIR_LEFT  = 3,
    OBJ_DIR_COUNT = 4
};

struct ObjectAnimClip {
    int frameCount;
    Rectangle frames[16];
};

struct ObjectSpriteSet {
    ObjectAnimClip dirs[OBJ_DIR_COUNT];
};

struct MapObject {
    int tileX;
    int tileY;
    int typeId;
    int layer;
    int dir;
    int currentFrame;
    int state;
    int integrity;     // 100 = цілий, 0 = викручений/перерізаний
    float animTimer;
    bool isOpen;
    bool isSolid;
};

class WorldObjects {
public:
    static const int MAX_OBJECTS = 1024;
    static const int SOURCE_TILE_SIZE = 16;
    static const int SCREEN_TILE_SIZE = 32;

    WorldObjects();
    ~WorldObjects();

    void Clear();
    void Unload();

    bool LoadAtlas(const char* texturePath, const char* configPath);
    bool AddObject(int tileX, int tileY, int typeId, int layer);
    void ResolveOrientations(const Tilemap& map);

    void Update(float deltaTime);
    void Draw(int activeLayer) const;
    void DrawDebug(int activeLayer) const;

    bool CheckCollisionAABB(Rectangle box, int layer) const;
    MapObject* GetObjectAt(int tileX, int tileY, int layer);
    const MapObject* GetObjectAt(int tileX, int tileY, int layer) const;

    bool IsDoorType(int typeId) const;
    bool IsDefaultSolidType(int typeId) const;
    bool IsClimbableType(int typeId) const;
    bool IsUnscrewableType(int typeId) const;
    bool IsCuttableType(int typeId) const;

    int GetCount() const { return objectCount; }
    const MapObject& GetByIndex(int index) const { return objects[index]; }

private:
    Texture2D objectsTexture;
    bool isTextureLoaded;
    ObjectSpriteSet spriteSets[256];

    bool anchorBottom[256];
    bool defaultSolid[256];
    bool doorTypes[256];
    bool climbableTypes[256];
    bool unscrewableTypes[256];
    bool cuttableTypes[256];

    MapObject objects[MAX_OBJECTS];
    int objectCount;

    static void ParseRangeList(const char* listStr, bool targetArray[256]);
};

#endif // WORLD_OBJECTS_HPP