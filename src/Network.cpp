#include "Network.hpp"
#include <enet/enet.h>
#include <cstring>
#include <iostream>

Vector2 NetworkManager::GetPlayerSpawnPosition(int playerIndex) {
    switch (playerIndex) {
        case 0: return (Vector2){ 1920.0f, 1216.0f };
        case 1: return (Vector2){ 2080.0f, 1216.0f }; 
        case 2: return (Vector2){ 2240.0f, 1216.0f };
        case 3: return (Vector2){ 1920.0f, 1440.0f };
        default: return (Vector2){ 1920.0f, 1216.0f };
    }
}

NetworkManager::NetworkManager() {
    mode = NET_MODE_NONE;
    isInitialized = false;
    isConnected = false;
    hostPtr = nullptr;
    serverPeerPtr = nullptr;
    localPlayerIndex = -1;
    connectedPlayersCount = 0;
    syncTimer = 0.0f;
    hasNewWorldState = false;
    memset(playerButtons, 0, sizeof(playerButtons));
    memset(&latestWorldState, 0, sizeof(latestWorldState));
}

NetworkManager::~NetworkManager() {
    Shutdown();
}

bool NetworkManager::Init() {
    if (isInitialized) return true;
    if (enet_initialize() != 0) {
        std::cerr << "[NET] Помилка ініціалізації ENet!" << std::endl;
        return false;
    }
    isInitialized = true;
    return true;
}

void NetworkManager::Shutdown() {
    if (serverPeerPtr != nullptr) {
        enet_peer_disconnect_now((ENetPeer*)serverPeerPtr, 0);
        serverPeerPtr = nullptr;
    }
    if (hostPtr != nullptr) {
        enet_host_destroy((ENetHost*)hostPtr);
        hostPtr = nullptr;
    }
    if (isInitialized) {
        enet_deinitialize();
        isInitialized = false;
    }
    mode = NET_MODE_NONE;
    isConnected = false;
    localPlayerIndex = -1;
    connectedPlayersCount = 0;
    memset(playerButtons, 0, sizeof(playerButtons));
}

bool NetworkManager::StartHost(Entity* entities, uint16_t port) {
    if (!Init()) return false;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    ENetHost* server = enet_host_create(&address, MAX_NET_PLAYERS - 1, 2, 0, 0);
    if (server == nullptr) {
        std::cerr << "[NET] Не вдалося створити сервер на порту " << port << std::endl;
        return false;
    }

    hostPtr = server;
    mode = NET_MODE_HOST;
    isConnected = true;
    localPlayerIndex = 0;
    connectedPlayersCount = 1;
    syncTimer = 0.0f;

    entities[0].isActive = true;
    entities[0].transform.position = GetPlayerSpawnPosition(0);

    std::cout << "[NET] Сервер запущено на порту " << port << " (слотів: 4)." << std::endl;
    return true;
}

bool NetworkManager::ConnectToHost(const char* ipAddress, uint16_t port) {
    if (!Init()) return false;

    ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (client == nullptr) {
        std::cerr << "[NET] Не вдалося створити клієнт ENet!" << std::endl;
        return false;
    }

    ENetAddress address;
    enet_address_set_host(&address, ipAddress);
    address.port = port;

    ENetPeer* peer = enet_host_connect(client, &address, 2, 0);
    if (peer == nullptr) {
        std::cerr << "[NET] Немає вільних слотів для підключення!" << std::endl;
        enet_host_destroy(client);
        return false;
    }

    hostPtr = client;
    serverPeerPtr = peer;
    mode = NET_MODE_CLIENT;
    isConnected = false;
    localPlayerIndex = -1;
    std::cout << "[NET] Підключення до " << ipAddress << ":" << port << "..." << std::endl;
    return true;
}

uint8_t NetworkManager::GetPlayerInput(int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= MAX_NET_PLAYERS) return 0;
    return playerButtons[playerIndex];
}

void NetworkManager::PollEvents(Entity* entities) {
    if (hostPtr == nullptr) return;

    ENetHost* host = (ENetHost*)hostPtr;
    ENetEvent event;

    while (enet_host_service(host, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                if (mode == NET_MODE_HOST) {
                    int assignedSlot = -1;
                    for (int i = 1; i < MAX_NET_PLAYERS; ++i) {
                        if (!entities[i].isActive) {
                            assignedSlot = i;
                            break;
                        }
                    }

                    if (assignedSlot == -1) {
                        std::cout << "[NET] Сервер переповнений, відхилено підключення." << std::endl;
                        enet_peer_disconnect_now(event.peer, 0);
                        break;
                    }

                    event.peer->data = (void*)(intptr_t)assignedSlot;

                    entities[assignedSlot].isActive = true;
                    entities[assignedSlot].currentLayer = 1;
                    entities[assignedSlot].isNoclip = false;
                    entities[assignedSlot].transform.position = GetPlayerSpawnPosition(assignedSlot);
                    if (entities[assignedSlot].hasHealth) {
                        entities[assignedSlot].health.currentHealth = entities[assignedSlot].health.maxHealth;
                    }
                    playerButtons[assignedSlot] = 0;
                    connectedPlayersCount++;

                    WelcomePacket welcomePkt = { PACKET_WELCOME, (uint8_t)assignedSlot };
                    ENetPacket* packet = enet_packet_create(&welcomePkt, sizeof(welcomePkt), ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(event.peer, 1, packet);

                    std::cout << "[NET] Гравець підключився у слот " << assignedSlot
                              << " (" << connectedPlayersCount << "/4)." << std::endl;
                }
                else if (mode == NET_MODE_CLIENT) {
                    isConnected = true;
                    std::cout << "[NET] З'єднано з сервером, очікування призначення слоту..." << std::endl;
                }
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                if (event.packet->dataLength >= 1) {
                    uint8_t packetType = event.packet->data[0];

                    if (mode == NET_MODE_HOST && packetType == PACKET_INPUT &&
                        event.packet->dataLength == sizeof(InputPacket)) {
                        int slot = (int)(intptr_t)event.peer->data;
                        if (slot >= 1 && slot < MAX_NET_PLAYERS) {
                            InputPacket* pkt = (InputPacket*)event.packet->data;
                            playerButtons[slot] = pkt->buttons;
                            entities[slot].currentLayer = pkt->layer;
                            entities[slot].isNoclip = (pkt->noclip != 0);
                        }
                    }
                    else if (mode == NET_MODE_CLIENT) {
                        if (packetType == PACKET_WELCOME && event.packet->dataLength == sizeof(WelcomePacket)) {
                            WelcomePacket* pkt = (WelcomePacket*)event.packet->data;
                            localPlayerIndex = pkt->playerIndex;
                            if (localPlayerIndex >= 0 && localPlayerIndex < MAX_NET_PLAYERS) {
                                entities[localPlayerIndex].isActive = true;
                            }
                            std::cout << "[NET] Отримано слот гравця: Player " << (localPlayerIndex + 1) << std::endl;
                        }
                        else if (packetType == PACKET_WORLD_STATE && event.packet->dataLength == sizeof(WorldStatePacket)) {
                            memcpy(&latestWorldState, event.packet->data, sizeof(WorldStatePacket));
                            hasNewWorldState = true;
                        }
                    }
                }
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                if (mode == NET_MODE_HOST) {
                    int slot = (int)(intptr_t)event.peer->data;
                    if (slot >= 1 && slot < MAX_NET_PLAYERS && entities[slot].isActive) {
                        entities[slot].isActive = false;
                        playerButtons[slot] = 0;
                        connectedPlayersCount--;
                        std::cout << "[NET] Гравець зі слоту " << slot
                                  << " вийшов (" << connectedPlayersCount << "/4)." << std::endl;
                    }
                    event.peer->data = nullptr;
                }
                else if (mode == NET_MODE_CLIENT) {
                    std::cout << "[NET] Відключено від сервера." << std::endl;
                    isConnected = false;
                    localPlayerIndex = -1;
                    serverPeerPtr = nullptr;
                }
                break;
            }

            default:
                break;
        }
    }
}

void NetworkManager::SendInput(uint8_t buttons, uint8_t layer, bool noclip) {
    if (mode != NET_MODE_CLIENT || !isConnected || localPlayerIndex < 0 || serverPeerPtr == nullptr) return;

    InputPacket pkt = { PACKET_INPUT, buttons, layer, (uint8_t)(noclip ? 1 : 0) };
    ENetPacket* packet = enet_packet_create(&pkt, sizeof(pkt), ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send((ENetPeer*)serverPeerPtr, 0, packet);
}

void NetworkManager::BroadcastWorldState(const Entity* entities, int entityCount) {
    if (mode != NET_MODE_HOST || hostPtr == nullptr || connectedPlayersCount <= 1) return;

    WorldStatePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.type = PACKET_WORLD_STATE;
    pkt.entityCount = (uint8_t)((entityCount < 32) ? entityCount : 32);

    for (int i = 0; i < pkt.entityCount; ++i) {
        const Entity& e = entities[i];
        pkt.entities[i].id = (int16_t)e.id;
        pkt.entities[i].x = e.transform.position.x;
        pkt.entities[i].y = e.transform.position.y;
        pkt.entities[i].currentHealth = e.hasHealth ? (int16_t)e.health.currentHealth : 100;
        pkt.entities[i].fatigue = e.hasPlayerNeeds ? (uint8_t)e.playerNeeds.fatigue : 0;
        pkt.entities[i].heat = e.hasPlayerNeeds ? (uint8_t)e.playerNeeds.heat : 0;
        pkt.entities[i].isActive = e.isActive ? 1 : 0;
        pkt.entities[i].currentLayer = (uint8_t)e.currentLayer;
    }

    ENetPacket* packet = enet_packet_create(&pkt, sizeof(pkt), ENET_PACKET_FLAG_UNSEQUENCED);
    enet_host_broadcast((ENetHost*)hostPtr, 0, packet);
}

void NetworkManager::ApplyWorldState(Entity* entities, int entityCount) const {
    int count = (latestWorldState.entityCount < entityCount) ? latestWorldState.entityCount : entityCount;

    for (int i = 0; i < count; ++i) {
        Entity& e = entities[i];
        const NetEntityState& net = latestWorldState.entities[i];

        e.transform.position.x = net.x;
        e.transform.position.y = net.y;
        e.isActive = (net.isActive != 0);
        if (i != localPlayerIndex) {
            e.currentLayer = net.currentLayer;
        }

        if (e.hasHealth) e.health.currentHealth = net.currentHealth;
        if (e.hasPlayerNeeds) {
            e.playerNeeds.fatigue = net.fatigue;
            e.playerNeeds.heat = net.heat;
        }
    }
}

void NetworkManager::Sync(Entity* entities, int entityCount, uint8_t localInputButtons, float deltaTime) {
    PollEvents(entities);

    if (mode == NET_MODE_HOST) {
        syncTimer += deltaTime;
        if (syncTimer >= (1.0f / 30.0f)) {
            syncTimer = 0.0f;
            BroadcastWorldState(entities, entityCount);
        }
    }
    else if (mode == NET_MODE_CLIENT) {
        uint8_t myLayer = 1;
        bool myNoclip = false;
        if (localPlayerIndex >= 0 && localPlayerIndex < entityCount) {
            myLayer = (uint8_t)entities[localPlayerIndex].currentLayer;
            myNoclip = entities[localPlayerIndex].isNoclip;
        }

        SendInput(localInputButtons, myLayer, myNoclip);
        if (hasNewWorldState) {
            ApplyWorldState(entities, entityCount);
            hasNewWorldState = false;
        }
    }
}