#pragma once
#include "Entity.hpp"
#include <cstdint>

enum NetworkMode {
    NET_MODE_NONE = 0,
    NET_MODE_HOST,
    NET_MODE_CLIENT
};

enum PacketType : uint8_t {
    PACKET_WELCOME     = 1,
    PACKET_INPUT       = 2,
    PACKET_WORLD_STATE = 3
};

#pragma pack(push, 1)

struct WelcomePacket {
    uint8_t type;
    uint8_t playerIndex;
};

struct InputPacket {
    uint8_t type; 
    uint8_t buttons;
    uint8_t layer;
    uint8_t noclip;
};

struct NetEntityState {
    int16_t id;
    float x;
    float y;
    int16_t currentHealth;
    uint8_t fatigue;
    uint8_t heat;
    uint8_t isActive;
    uint8_t currentLayer;
};

struct WorldStatePacket {
    uint8_t type;
    uint8_t entityCount;
    NetEntityState entities[32];
};

#pragma pack(pop)

class NetworkManager {
public:
    static const int MAX_NET_PLAYERS = 4;
    static const uint16_t DEFAULT_PORT = 7777;

    NetworkManager();
    ~NetworkManager();

    bool Init();
    void Shutdown();

    bool StartHost(Entity* entities, uint16_t port = DEFAULT_PORT);
    bool ConnectToHost(const char* ipAddress, uint16_t port = DEFAULT_PORT);

    void Sync(Entity* entities, int entityCount, uint8_t localInputButtons, float deltaTime);

    NetworkMode GetMode() const { return mode; }
    bool IsConnected() const { return isConnected; }
    int GetConnectedCount() const { return connectedPlayersCount; }
    int GetLocalPlayerIndex() const { return localPlayerIndex; }
    uint8_t GetPlayerInput(int playerIndex) const;

    static Vector2 GetPlayerSpawnPosition(int playerIndex);

private:
    void PollEvents(Entity* entities);
    void SendInput(uint8_t buttons, uint8_t layer, bool noclip);
    void BroadcastWorldState(const Entity* entities, int entityCount);
    void ApplyWorldState(Entity* entities, int entityCount) const;

    NetworkMode mode;
    bool isInitialized;
    bool isConnected;

    void* hostPtr;
    void* serverPeerPtr; 

    int localPlayerIndex; 
    int connectedPlayersCount;
    uint8_t playerButtons[MAX_NET_PLAYERS];

    float syncTimer;
    WorldStatePacket latestWorldState;
    bool hasNewWorldState;
};