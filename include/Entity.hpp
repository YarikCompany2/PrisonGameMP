#pragma once
#include "raylib.h"
#include "Weapon.hpp"
#include "Outfit.hpp"
#include <cstring>

enum EntityType {
    ENTITY_TYPE_PRISONER,
    ENTITY_TYPE_OFFICER,
    ENTITY_TYPE_CIVILIAN
};

enum ControllerType {
    CONTROLLER_LOCAL_PLAYER,   
    CONTROLLER_NETWORK_PLAYER, 
    CONTROLLER_AI,
    CONTROLLER_NONE
};

enum PrisonJobType {
    JOB_UNEMPLOYED,
    JOB_JANITOR,
    JOB_LAUNDRY,
    JOB_KITCHEN,
    JOB_WOODSHOP,
    JOB_METALSHOP
};

struct Transform2D {
    Vector2 position;
    Vector2 size;
};

struct HealthStats {
    int maxHealth;
    int currentHealth;
};

struct CharacterAttributes {
    int strength;
    int speed;
    int intellect;
};

struct SocialStats {
    int opinion;
};

struct Equipment {
    Outfit currentOutfit;
    Weapon currentWeapon;
};

struct JobComponent {
    PrisonJobType jobType;
    int quotaProgress;
    int quotaRequired;
};

struct PlayerNeeds {
    int fatigue;
    int heat;
};

class Entity {
public:
    int id;
    char name[32];
    EntityType type;
    ControllerType controller;

    Transform2D transform;
    Color color;

    int currentLayer;
    bool isNoclip;

    bool hasHealth;
    HealthStats health;

    bool hasAttributes;
    CharacterAttributes attributes;

    bool hasEquipment;
    Equipment equipment;

    bool hasSocialStats;
    SocialStats social;

    bool hasJob;
    JobComponent job;

    bool hasPlayerNeeds;
    PlayerNeeds playerNeeds;

    bool isActive;

    Entity() {
        id = -1;
        name[0] = '\0';
        type = ENTITY_TYPE_CIVILIAN;
        controller = CONTROLLER_NONE;
        
        transform.position = (Vector2){ 0.0f, 0.0f };
        transform.size = (Vector2){ 28.0f, 28.0f };
        color = WHITE;

        currentLayer = 1; // Tilemap::LAYER_GROUND
        isNoclip = false;

        hasHealth = false;
        hasAttributes = false;
        hasEquipment = false;
        hasSocialStats = false;
        hasJob = false;
        hasPlayerNeeds = false;

        isActive = false;
    }

    bool IsAlive() const {
        if (!hasHealth) return true;
        return health.currentHealth > 0;
    }

    void TakeDamage(int damageAmount) {
        if (!hasHealth) return;

        int effectiveDamage = damageAmount;
        if (hasEquipment) {
            effectiveDamage -= equipment.currentOutfit.defense;
            if (effectiveDamage < 1) effectiveDamage = 1;
        }

        health.currentHealth -= effectiveDamage;
        if (health.currentHealth < 0) {
            health.currentHealth = 0;
        }
    }
};

namespace EntityFactory {

    inline Entity CreatePlayer(int id, const char* playerName, Vector2 startPosition, Color playerColor, ControllerType controllerType) {
        Entity entity;
        entity.id = id;
        strncpy(entity.name, playerName, sizeof(entity.name) - 1);
        entity.type = ENTITY_TYPE_PRISONER;
        entity.controller = controllerType;

        entity.transform.position = startPosition;
        entity.transform.size = (Vector2){ 28.0f, 28.0f };
        entity.color = playerColor;

        entity.currentLayer = 1;
        entity.isNoclip = false;
 
        entity.hasHealth = true;
        entity.health.maxHealth = 100;
        entity.health.currentHealth = 100;

        entity.hasAttributes = true;
        entity.attributes.strength = 20;
        entity.attributes.speed = 20;
        entity.attributes.intellect = 20;

        entity.hasEquipment = true;
        entity.equipment.currentOutfit = Outfit::CreateInmateOutfit();
        entity.equipment.currentWeapon = Weapon::CreateFists();

        entity.hasJob = true;
        entity.job.jobType = JOB_JANITOR;
        entity.job.quotaProgress = 0;
        entity.job.quotaRequired = 5;

        entity.hasPlayerNeeds = true;
        entity.playerNeeds.fatigue = 0;
        entity.playerNeeds.heat = 0;

        entity.hasSocialStats = false;

        entity.isActive = true;
        return entity;
    }

    inline Entity CreateNpcPrisoner(int id, const char* prisonerName, Vector2 startPosition) {
        Entity entity;
        entity.id = id;
        strncpy(entity.name, prisonerName, sizeof(entity.name) - 1);
        entity.type = ENTITY_TYPE_PRISONER;
        entity.controller = CONTROLLER_AI;

        entity.transform.position = startPosition;
        entity.transform.size = (Vector2){ 28.0f, 28.0f };
        entity.color = ORANGE;

        entity.currentLayer = 1;
        entity.isNoclip = false;

        entity.hasHealth = true;
        entity.health.maxHealth = 100;
        entity.health.currentHealth = 100;

        entity.hasAttributes = true;
        entity.attributes.strength = 25;
        entity.attributes.speed = 20;
        entity.attributes.intellect = 15;

        entity.hasEquipment = true;
        entity.equipment.currentOutfit = Outfit::CreateInmateOutfit();
        entity.equipment.currentWeapon = Weapon::CreateFists();

        entity.hasJob = true;
        entity.job.jobType = JOB_LAUNDRY;
        entity.job.quotaProgress = 0;
        entity.job.quotaRequired = 5;

        entity.hasSocialStats = true;
        entity.social.opinion = 50;

        entity.hasPlayerNeeds = false;

        entity.isActive = true;
        return entity;
    }

    inline Entity CreateOfficer(int id, Vector2 startPosition) {
        Entity entity;
        entity.id = id;
        strncpy(entity.name, "Officer", sizeof(entity.name) - 1);
        entity.type = ENTITY_TYPE_OFFICER;
        entity.controller = CONTROLLER_AI;

        entity.transform.position = startPosition;
        entity.transform.size = (Vector2){ 28.0f, 28.0f };
        entity.color = DARKBLUE;

        entity.currentLayer = 1;
        entity.isNoclip = false;

        entity.hasHealth = true;
        entity.health.maxHealth = 150;
        entity.health.currentHealth = 150;

        entity.hasAttributes = true;
        entity.attributes.strength = 40;
        entity.attributes.speed = 30;
        entity.attributes.intellect = 30;

        entity.hasEquipment = true;
        entity.equipment.currentOutfit = Outfit::CreateGuardOutfit();
        entity.equipment.currentWeapon = Weapon::CreateBaton();

        entity.hasSocialStats = true;
        entity.social.opinion = 60;

        entity.hasJob = false;
        entity.hasPlayerNeeds = false;

        entity.isActive = true;
        return entity;
    }

    inline Entity CreateCivilian(int id, const char* civilianName, Vector2 startPosition) {
        Entity entity;
        entity.id = id;
        strncpy(entity.name, civilianName, sizeof(entity.name) - 1);
        entity.type = ENTITY_TYPE_CIVILIAN;
        entity.controller = CONTROLLER_NONE;

        entity.transform.position = startPosition;
        entity.transform.size = (Vector2){ 28.0f, 28.0f };
        entity.color = PURPLE;

        entity.currentLayer = 1;
        entity.isNoclip = false;

        entity.hasHealth = false;
        entity.hasAttributes = false;
        entity.hasEquipment = false;
        entity.hasSocialStats = false;
        entity.hasJob = false;
        entity.hasPlayerNeeds = false;

        entity.isActive = true;
        return entity;
    }
}