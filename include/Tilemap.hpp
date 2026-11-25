#ifndef TILEMAP_HPP
#define TILEMAP_HPP

#include <raylib.h>

class WorldObjects;

struct TileProperties {
    bool isSolid;
    bool isCuttable;
    bool isChippable;
};

class Tilemap {
public:
    static const int MAX_MAP_WIDTH = 128;
    static const int MAX_MAP_HEIGHT = 128;
    static const int SOURCE_TILE_SIZE = 16;
    static const int SCREEN_TILE_SIZE = 32;

    enum MapLayer : int {
        LAYER_UNDERGROUND = 0,
        LAYER_GROUND      = 1,
        LAYER_VENTS       = 2,
        LAYER_ROOF        = 3,
        LAYER_COUNT       = 4
    };

    Tilemap();
    ~Tilemap();

    bool LoadFromCmap(const char* filePath, WorldObjects* outObjects = nullptr);
    bool LoadTileset(const char* texturePath);
    bool LoadTilesetConfig(const char* configPath);
    void Unload();

    void Draw(int activeLayer = LAYER_GROUND, float alpha = 1.0f) const;
    void DrawSolidDebug(int activeLayer = LAYER_GROUND) const;

    bool IsSolidAt(int gridColumn, int gridRow, int layer = LAYER_GROUND) const;
    void SetSolidAt(int gridColumn, int gridRow, bool solid, int layer = LAYER_GROUND);
    bool CheckCollisionAABB(Rectangle box, int layer = LAYER_GROUND) const;

    int GetWidth() const { return mapWidth; }
    int GetHeight() const { return mapHeight; }
    const char* GetPrisonName() const { return prisonName; }

private:
    int mapWidth;
    int mapHeight;
    char prisonName[64];
    char tilesetName[32];

    int tiles[LAYER_COUNT][MAX_MAP_HEIGHT][MAX_MAP_WIDTH];
    bool solidGrid[LAYER_COUNT][MAX_MAP_HEIGHT][MAX_MAP_WIDTH];
    TileProperties tileDatabase[256];

    Texture2D tilesetTexture;
    bool isTextureLoaded;

    void ResetTileDatabase();
    void ApplyRangeListToDatabase(const char* listStr, int flagType);
    void RebuildSolidGrid();
};

#endif // TILEMAP_HPP