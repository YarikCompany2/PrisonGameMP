#pragma once
#include <cstring>

class Weapon {
public:
    int id;
    char name[32];
    int damage;
    int maxDurability;
    int currentDurability;
    bool isContraband;

    Weapon() {
        id = 0;
        strncpy(name, "Fists", sizeof(name) - 1);
        damage = 5;
        maxDurability = -1;
        currentDurability = -1;
        isContraband = false;
    }

    Weapon(int weaponId, const char* weaponName, int attackDamage, int durability, bool contrabandStatus) {
        id = weaponId;
        strncpy(name, weaponName, sizeof(name) - 1);
        damage = attackDamage;
        maxDurability = durability;
        currentDurability = durability;
        isContraband = contrabandStatus;
    }

    void OnHit() {
        if (maxDurability == -1) return;

        if (currentDurability > 0) {
            currentDurability--;
        }
    }

    bool IsBroken() const {
        if (maxDurability == -1) return false;
        return currentDurability <= 0;
    }

    static Weapon CreateFists() {
        return Weapon(0, "Fists", 5, -1, false);
    }

    static Weapon CreateSockMace() {
        return Weapon(1, "Sock Mace", 15, 12, true);
    }

    static Weapon CreateGlassShank() {
        return Weapon(2, "Glass Shank", 25, 8, true);
    }

    static Weapon CreateBaton() {
        return Weapon(3, "Guard Baton", 20, 25, true);
    }

    static Weapon CreateCupOfHotChocolate() {
        return Weapon(4, "Cup of Hot Chocolate", 80, 1, true);
    }

    static Weapon CreateNunchucks() {
        return Weapon(5, "Nunchucks", 40, 30, true);
    }
};